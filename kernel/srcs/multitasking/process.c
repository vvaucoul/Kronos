/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/02/12 10:13:19 by vvaucoul          #+#    #+#             */
/*   Updated: 2023/07/20 23:37:19 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <memory/frames.h>
#include <memory/kheap.h>
#include <memory/memory.h>

#include <multitasking/process.h>
#include <multitasking/scheduler.h>

#include <system/tss.h>

#include <system/time.h>

#include <asm/asm.h>

#include <kernel.h>

task_t *current_task = NULL;
// volatile task_t *scheduler_tasks[MAX_TASKS] = {NULL};
uint32_t num_tasks = 0;

task_t *ready_queue = NULL;
task_t *wait_queue = NULL;

extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

extern uint32_t read_eip(void);

extern uint32_t initial_esp;

static int32_t next_pid = 1;

static void move_stack(void *new_stack_start, uint32_t size) {
    uint32_t i, pd_addr, old_stack_pointer, old_base_pointer, new_stack_pointer, new_base_pointer, offset, tmp, *tmp2;

    /* Allocate some space for the new stack */
    for (i = (uint32_t)new_stack_start; i >= ((uint32_t)new_stack_start - size); i -= 0x1000) {
        /* General-purpose stack is in user-mode */
        const page_t *page = get_page(i, current_directory);
        if (!page) {
            page = create_page(i, current_directory);
        }
        alloc_frame((page_t *)page, 0, 1);
    }

    /* Flush the TLB by reading and writing the page directory address again */
    __asm__ __volatile__("mov %%cr3, %0"
                         : "=r"(pd_addr));
    __asm__ __volatile__("mov %0, %%cr3"
                         :
                         : "r"(pd_addr));

    /* Old ESP and EBP, read from registers */
    __asm__ __volatile__("mov %%esp, %0"
                         : "=r"(old_stack_pointer));
    __asm__ __volatile__("mov %%ebp, %0"
                         : "=r"(old_base_pointer));

    /* Offset to add to old stack addresses to get a new stack address */
    offset = (uint32_t)new_stack_start - initial_esp;

    /* New ESP and EBP */
    new_stack_pointer = old_stack_pointer + offset;
    new_base_pointer = old_base_pointer + offset;

    /* Copy the stack */
    memcpy((void *)new_stack_pointer, (void *)old_stack_pointer, initial_esp - old_stack_pointer);

    /* Backtrace through the original stack, copying new values into the new stack */
    for (i = (uint32_t)new_stack_start; i > (uint32_t)new_stack_start - size; i -= 4) {
        tmp = *(uint32_t *)i;
        if ((old_stack_pointer < tmp) && (tmp < initial_esp)) {
            tmp = tmp + offset;
            tmp2 = (uint32_t *)i;
            *tmp2 = tmp;
        }
    }

    /* Change stacks */
    __asm__ __volatile__("mov %0, %%esp"
                         :
                         : "r"(new_stack_pointer));
    __asm__ __volatile__("mov %0, %%ebp"
                         :
                         : "r"(new_base_pointer));
}

void init_tasking(void) {
    // printk("\n\n--- INIT TASKING ---\n\n");

    ASM_CLI();

    // printk("\t- Move stack\n");

    move_stack((void *)0xE0000000, 0x2000);

    /* Initialise the first task (kernel task) */
    current_task = ready_queue = (task_t *)kmalloc(sizeof(task_t));

    if (!(current_task))
        __THROW_NO_RETURN("init_tasking : kmalloc failed");

    current_task->pid = next_pid++;
    current_task->ppid = 0;
    current_task->esp = current_task->ebp = 0;
    current_task->eip = 0;
    current_task->page_directory = current_directory;
    current_task->next = current_task->prev = NULL;
    current_task->kernel_stack = (uint32_t)kmalloc_a(KERNEL_STACK_SIZE);
    current_task->exit_code = 0;
    current_task->state = TASK_RUNNING;
    current_task->owner = 0;
    current_task->tid = (task_id_t){0, 0, 0, 0};
    current_task->cpu_load = (process_cpu_load_t){0, 0, 0};
    // Set CPU load start time
    // uint64_t start_time;
    // tm_t time = gettime();
    // start_time = mktime(&time);
    // current_task->cpu_load.start_time = start_time;
    // current_task->cpu_load.load_time = 0;

    if (!(current_task->kernel_stack))
        __THROW_NO_RETURN("init_tasking : kmalloc_a failed");

    // printk("\t- Current task : %d\n", current_task->pid);

    wait_queue = NULL;

    ASM_STI();
}

int32_t task_fork(void) {
    // printk("\t- Fork\n");

    uint32_t eip, esp, ebp;
    task_t *parent_task, *new_task, *tmp_task;
    page_directory_t *directory;

    /* We are modifying kernel structures, and so cannot preempt */
    ASM_CLI();

    /* Take a pointer to this process' task struct for later reference */
    parent_task = (task_t *)current_task;
    // printk("\t- Parent task : %d\n", parent_task->pid);

    /* Clone the address space */
    if (!(directory = clone_page_directory(current_directory)))
        __THROW("task_fork : clone_page_directory failed", 1);

    /* Create a new process */
    if (!(new_task = (task_t *)kmalloc(sizeof(task_t))))
        __THROW("task_fork : kmalloc failed", 1);

    new_task->pid = next_pid++;
    new_task->esp = new_task->ebp = 0;
    new_task->eip = 0;
    new_task->page_directory = directory;
    new_task->kernel_stack = (uint32_t)kmalloc_a(KERNEL_STACK_SIZE);
    new_task->next = NULL;
    new_task->prev = NULL; // Set prev task when added to ready queue
    new_task->exit_code = 0;
    new_task->state = TASK_RUNNING;
    new_task->owner = 0;
    new_task->tid = (task_id_t){0, 0, 0, 0};
    new_task->cpu_load = (process_cpu_load_t){0, 0, 0};

    // Set CPU load start time
    // uint64_t start_time;
    // tm_t time = gettime();
    // start_time = mktime(&time);
    // current_task->cpu_load.start_time = start_time;
    // current_task->cpu_load.load_time = 0;

    if (!(current_task->kernel_stack))
        __THROW("task_fork : kmalloc failed", 1);

    /* Add it to the end of the ready queue */
    tmp_task = (task_t *)ready_queue;
    while (tmp_task->next)
        tmp_task = tmp_task->next;
    tmp_task->next = new_task;
    new_task->prev = tmp_task;

    // printk("\t- Prev Task [%d] -> Task [%d] -> Next Task [%d]\n", tmp_task->pid, new_task->pid, new_task->next == NULL ? -1 : new_task->next->pid);

    /* This will be the entry point for the new process */
    eip = read_eip();

    // printk("\t- EIP : %x\n", eip);
    // printk("\t- current_task == parent_task : %d\n", current_task == parent_task);

    /* We could be the parent or the child here - check */
    if (current_task == parent_task) {
        /* We are the parent, so set up the esp/ebp/eip for our child */
        __asm__ __volatile__("mov %%esp, %0"
                             : "=r"(esp));
        __asm__ __volatile__("mov %%ebp, %0"
                             : "=r"(ebp));
        new_task->esp = esp;
        new_task->ebp = ebp;
        new_task->eip = eip;
        new_task->ppid = parent_task->pid;

        ASM_STI();

        return (new_task->pid);
    } else {
        /* We are the child */
        ASM_STI();
        return 0;
    }
}

__attribute__((pure)) page_directory_t *get_task_directory(void) {
    return current_task->page_directory;
}

void set_task_uid(task_t *task, uint32_t uid) {
    task->tid.uid = uid;
}

void set_task_gid(task_t *task, uint32_t gid) {
    task->tid.gid = gid;
}

void set_task_euid(task_t *task, uint32_t euid) {
    task->tid.euid = euid;
}

void set_task_egid(task_t *task, uint32_t egid) {
    task->tid.egid = egid;
}

pid_t getpid(void) {
    return current_task->pid;
}

int32_t getppid(void) {
    return current_task->ppid;
}

task_t *get_current_task(void) {
    return (task_t *)current_task;
}

task_t *get_task(int32_t pid) {
    task_t *tmp_task = (task_t *)ready_queue;

    if (!tmp_task) {
        return NULL;
    }

    /* Find the process in the proc. list */
    while (tmp_task->pid != pid) {
        tmp_task = tmp_task->next;
        if (!tmp_task)
            break;
    }
    return tmp_task;
}

int32_t init_task(void func(void)) {
    // printk("\n\n--- INIT TASK ---\n\n");

    int32_t ret = task_fork();
    int32_t pid = getpid();

    /* If we are the child */
    if (!ret) {
        /* Execute the requested function */
        func();
        /* Set task to running state */
        // get_task(pid)->state = TASK_RUNNING;
        /* Kill the current (child) process. On failure, freeze it */
        // printk("Child process returned from init_task\n");
        if (kill_task(pid) != 0) {
            // printk("Child process failed to die, freezing\n");
            for (;;)
                ;
        }
        // printk("Child process failed to die\n");
    }
    return ret;
}

int32_t task_wait(int32_t pid) {
    ASM_CLI();
    task_t *task = get_task(pid);
    if (!task) {
        printk("Invalid PID: %d\n", pid);
        return -1;
    }

    // Wait for the task to finish
    while (task->state == TASK_RUNNING) {
        ksleep(1);
    }

    // Task has finished, so clean it up
    int32_t exit_code = task->exit_code;
    kill_task(pid);

    ASM_STI();
    return exit_code;
}

int32_t kill_task(int32_t pid) {
    task_t *tmp_task;
    task_t *par_task;

    if (!pid) {
        return 0;
    }

    tmp_task = get_task(pid);
    if (!tmp_task) {
        __THROW("kill_task : task not found for pid %d", -1, pid);
    }

    /* Can we delete it? */
    if (tmp_task->ppid != 0) {
        par_task = get_task(tmp_task->ppid);

        /* If its stack is reachable, delete it */
        if (tmp_task->kernel_stack) {
            kfree((void *)tmp_task->kernel_stack);
        }
        // todo: free page directory
        par_task->next = tmp_task->next;
        kfree((void *)tmp_task);
        ksleep(1);
        return (pid);
    } else {
        printk("Cannot kill task %d\n", pid);
        return (0);
    }
}

void lock_task(task_t *task) {
    ASM_CLI();
    task->state = TASK_WAITING;
    task->next = wait_queue;
    wait_queue = task;
    ASM_STI();
}

void unlock_task(task_t *task) {
    ASM_CLI();
    if (wait_queue) {
        task_t *tmp = wait_queue;
        if (tmp == task) {
            wait_queue = tmp->next;
        } else {
            while (tmp->next && tmp->next != task)
                tmp = tmp->next;
            if (tmp->next) {
                tmp->next = task->next;
            }
        }
        task->next = NULL;
        task->state = TASK_RUNNING;
    }
    ASM_STI();
}

void task_exit(int32_t retval) {
    current_task->exit_code = retval;
    current_task->state = TASK_ZOMBIE;
    // kill_task(current_task->pid);
}

void switch_to_user_mode(void) {
    tss_set_stack_pointer(current_task->kernel_stack + KERNEL_STACK_SIZE);

    /* Set up a stack structure for switching to user mode */
    __asm__ __volatile__("cli; \
	mov $0x23, %ax; \
	mov %ax, %ds; \
	mov %ax, %es; \
	mov %ax, %fs; \
	mov %ax, %gs; \
	mov %esp, %eax; \
	pushl $0x23; \
	pushl %esp; \
	pushf; \
	pushl $0x1B; \
	push $1f; \
	sti; \
	iret; \
	1: \
	");
}

uint32_t getuid(void) {
    return current_task->owner;
}

bool is_pid_valid(int pid) {
    return get_task(pid) != NULL;
}

task_t *get_wait_queue(void) {
    return wait_queue;
}

double get_cpu_load(task_t *task) {

    uint64_t sys_time = get_system_time();
    uint64_t elapsed_time = sys_time - task->cpu_load.last_start_time;

    if (elapsed_time == 0) {
        // The task has just been created and hasn't had a chance to run yet
        return 0.0;
    }

    // if (task->cpu_load.load_time < elapsed_time)
    //     __THROW("get_cpu_load : load_time < elapsed_time", 0.0);

    return (double)task->cpu_load.load_time / elapsed_time;
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                      UTILS                                     ||
// ! ||--------------------------------------------------------------------------------||

void print_task_info(task_t *task) {
    printk("Task PID: "_GREEN
           "[%d]"_END
           ", Parent PID: "_GREEN
           "[%d]"_END
           ", Owner: "_GREEN
           "[%d]"_END
           ", State: "_GREEN
           "[%d]"_END
           "\n",
           task->pid, task->ppid, task->owner, task->state);
}

void print_all_tasks() {
    task_t *task = ready_queue;
    while (task) {
        print_task_info(task);
        task = task->next;
    }
}

void print_parent_and_children(int pid) {
    task_t *parent_task = get_task(pid);
    if (!parent_task) {
        printk("Invalid PID: %d\n", pid);
        return;
    }

    print_task_info(parent_task);

    printk("Children:\n");
    task_t *task = ready_queue;
    while (task) {
        if (task->ppid == pid) {
            print_task_info(task);
        }
        task = task->next;
    }
}
