/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/12/07 22:33:43 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/23 16:04:39 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <multitasking/scheduler.h>

#include <system/io.h>
#include <system/pit.h>
#include <system/time.h>
#include <system/tss.h>

extern task_t *current_task;
extern task_t *ready_queue;

static task_t *prev_task = NULL;

/* Import Waiting Queue from scheduler.h:
** Defined in 'process.c'
*/
extern task_t *waiting_queue;

bool scheduler_initialized = false;

void init_scheduler(void) {
	scheduler_initialized = true;
}

/**
 * @brief Wait for scheduler to round
 * @note : Wait for the scheduler to round before switching task
 *         This is used to avoid switching task while the scheduler is rounding
 *         and avoid some bugs
 */
// Todo: Debug, this function need to wait for the scheduler to round
void wait_for_scheduler_rounded(void) {

	pid_t current_pid = get_current_task()->pid;
	pid_t next_pid;

	do {
		__asm__ volatile("int $0x20");
		next_pid = get_current_task()->pid;
	} while (next_pid != current_pid);

	// bool any_task_has_signal;

	// do {
	//     // Wait for the scheduler to round
	//     while (!scheduler_rounded) {
	//         __asm__ volatile("hlt");
	//     }

	//     // Reset the flag for subsequent checks
	//     scheduler_rounded = false;

	//     any_task_has_signal = false;
	//     task_t *tmp = ready_queue;

	//     if (!tmp) {
	//         printk("Queue is empty!\n");
	//         return;
	//     }

	//     printk("Ready Queue:\n");
	//     // Check all tasks for pending signals
	//     do {
	//         // __signal_handler(tmp);
	//         if (tmp && tmp->state == TASK_RUNNING && tmp->signal_queue != NULL) {
	//             any_task_has_signal = true;
	//             printk("Task %d has signal\n", tmp->pid);
	//             break;
	//         }
	//         else
	//             printk("Task %d has no signal\n", tmp->pid);
	//         tmp = tmp->next;

	//         if (!tmp) {
	//             printk("Queue is empty!\n");
	//             return;
	//         }
	//     } while (tmp != ready_queue);

	// } while (any_task_has_signal);
}

void switch_task(void) {
	if (!scheduler_initialized)
		return;

	outportb(0x20, 0x20); // Send EOI to PIC

	uint32_t esp, ebp, eip;

	/* If we haven't initialised tasking yet, just return */
	if (!current_task)
		return;

	/* Read esp, ebp now for saving later on */
	__asm__ __volatile__("mov %%esp, %0"
						 : "=r"(esp));
	__asm__ __volatile__("mov %%ebp, %0"
						 : "=r"(ebp));

	eip = read_eip();

	/* Have we just switched tasks? */
	if (eip == 0x12345)
		return;

	// if (d_page_directory) {
	//     destroy_page_directory(d_page_directory);
	//     d_page_directory = NULL;
	// }

	// printk("Current Task: %d\n", current_task->pid);

	// Just before we switch away from the current task, update its cpu_time
	if (prev_task) {
		// uint64_t sys_time = get_system_time();

		// prev_task->cpu_load.load_time = sys_time - prev_task->cpu_load.start_time;
		prev_task->cpu_load.load_time += pit_get_subticks();
		prev_task->cpu_load.total_load_time += prev_task->cpu_load.load_time;

		// if (prev_task->pid > 1) {
		//     printk("Time: %lu | task start time %lu\n", sys_time, prev_task->cpu_load.start_time);

		//     printk("- START Task %d: %lu\n", prev_task->pid, prev_task->cpu_load.start_time);
		//     printk("- LOAD Task %d: %lu\n", prev_task->pid, prev_task->cpu_load.load_time);
		//     printk("- CPU  Task %d: %lu\n", prev_task->pid, get_cpu_load(prev_task));
		//     printk("- TOTAL Task %d: %lu\n", prev_task->pid, prev_task->cpu_load.total_load_time);
		// }
	}

	/* No, we didn't switch tasks. Let's save some register values and switch */
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;

	/* Get the next task to run */
	if (prev_task && current_task == prev_task)
		current_task = current_task->next;
	if (!current_task)
		current_task = ready_queue;

	current_task = __process_selector(current_task);

	// TODO: Debug
	// if (current_task && current_task->state == TASK_STOPPED) {
	//     kill_task(current_task->pid);
	// }

	// printk("Task = %d | State = %d\n", current_task->pid, current_task->state);

	// printk("Switching to task %d\n", current_task->pid);

	/* If we fell off the end of the linked list start again at the beginning */
	if (!current_task)
		current_task = ready_queue;

	// printk("Switching to task %d\n", current_task->pid);

	/* Save the context of our current task */
	prev_task = current_task;

	// Just after we've switched to the new task, update its start_time
	current_task->cpu_load.start_time = get_system_time();
	prev_task->cpu_load.load_time = 0;

	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;

	/* Make sure the memory manager knows we've changed page directory */
	// mmu_get_current_directory() = current_task->page_directory;
	mmu_set_current_directory(current_task->page_directory);

	/* Change kernel stack over */
	tss_set_stack_pointer(current_task->kernel_stack + KERNEL_STACK_SIZE);

	// check if tasks < MAX_TASKS then, add task to ready_queue
	__process_waiting();

	// Todo: ...
	/* Check if the current task has overflowed its stack */
	__task_overflow_handler();

	/* Kill stopped tasks */
	int32_t killed_task = __process_killer();

	// We just killed current task
	if (killed_task) {
		printk("Killed task [%d]\n", killed_task);
		return;
	}

	/* Check if the current task is a zombie */
	__process_zombie(current_task);

	/* Check if the current task has received a signal */
	__signal_handler(current_task);

	/* Check if the current task is sleeping */
	__process_sleeping(current_task);

	/* Revive Zombies / Orphans tasks and attach them to the INIT task (Like UNIX System) */
	__orphans_collector(current_task);

	/* Check if the current task has threads, then switch to the runnable thread */
	__process_threads(current_task);

	/* Do not execute Zombie or Stopped tasks */
	if (current_task && (current_task->state == TASK_ZOMBIE || current_task->state == TASK_STOPPED))
		return;

	/* Switch to the new task's kernel stack */
	/* 0x12345: just a magic number */
	__asm__ __volatile__("		\
    cli;			\
	mov %0, %%ecx;		\
	mov %1, %%esp;		\
	mov %2, %%ebp;		\
	mov %3, %%cr3;		\
	mov $0x12345, %%eax;	\
	sti;			\
	jmp *%%ecx		"
						 :
						 : "r"(eip), "r"(esp), "r"(ebp), "r"(mmu_get_current_directory()->physicalAddr));
}
