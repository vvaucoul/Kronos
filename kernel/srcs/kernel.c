/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kernel.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 13:55:07 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:49:08 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <hephaistos.h>
#include <kernel.h>

/* KronOS Inlcudes */
#include <kronos/stack_monitor.h>

#include <shell/ksh.h>

/* Multitasking */
#include <multitasking/process.h>
#include <multitasking/scheduler.h>

/* System */
#include <system/bsod.h>
#include <system/cmos.h>
#include <system/cpu.h>
#include <system/fpu.h>
#include <system/gdt.h>
#include <system/idt.h>
#include <system/ipc.h>
#include <system/irq.h>
#include <system/isr.h>
#include <system/kerrno.h>
#include <system/mutex.h>
#include <system/panic.h>
#include <system/pit.h>
#include <system/random.h>
#include <system/serial.h>
#include <system/signal.h>
#include <system/threads.h>
#include <system/time.h>
#include <system/tss.h>

/* Syscall */
#include <syscall/syscall.h>

/* Drivers */
#include <drivers/device/char.h>
#include <drivers/device/devices.h>
#include <drivers/device/floppy.h>
#include <drivers/device/ide.h>
#include <drivers/display.h>
#include <drivers/keyboard.h>
#include <drivers/ps2/PS2_8042.h>
#include <drivers/usb.h>

#include <multiboot/multiboot.h>
#include <multiboot/multiboot_mmap.h>

/* Memory */
#include <mm/ealloc.h>
#include <mm/mm.h>
#include <mm/mmap.h>
#include <mm/mmu.h>

/* Filesystem */
#include <fs/ext2/ext2.h>
#include <fs/initrd/initrd.h>
#include <fs/tinyfs/tinyfs.h>
#include <fs/vfs/vfs.h>

#include <system/fs/open.h>

#include <workflows/workflows.h>

/* Apps */
#include <apps/kfe/kfe.h>

/* Libs Import */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if __HIGHER_HALF_KERNEL__ == true
#error "Higher Half Kernel is not supported yet"
#endif

uint32_t initial_esp;
uint32_t *kernel_stack;

// ! ||--------------------------------------------------------------------------------||
// ! ||                             KERNEL UTILS FUNCTIONS                             ||
// ! ||--------------------------------------------------------------------------------||

static inline void
ksh_header(void) {
	const char *header_lines[] = {
		"##   ###   ##",
		" ##  ###  ## ",
		"  ## ### ##  ",
		"  ## ### ##  ",
		"  ## ### ##  ",
		" ##  ###  ## ",
		"##   ###   ##"};

	printk(_RED "\n");
	for (int i = 0; i < sizeof(header_lines) / sizeof(header_lines[0]); ++i) {
		printk("\t\t\t\t\t\t\t\t%s\n", header_lines[i]);
	}
	printk("\n");
	for (uint8_t i = 0; i < 80; ++i) {
		putchar('#');
	}

	printk(_END "\n");

	printk(_LCYAN "Kernel Version: " _LGREEN __KERNEL_VERSION__ "\n");
	printk(_LCYAN "Kernel Name: " _LGREEN __KERNEL_NAME__ "\n");
	printk(_LCYAN "Compiler: " _LGREEN __COMPILER__ "\n");
	printk(_LCYAN "Author: " _LGREEN __AUTHOR__ "\n");

	printk(_END "\n" _RED);

	for (uint8_t i = 0; i < 80; ++i) {
		putchar('#');
	}
	printk(_END "\n");
}

void kernel_log_info(const char *part, const char *name) {
	if (__DISPLAY_INIT_LOG__) {
		if (part == NULL || name == NULL) {
			return;
		}

		tm_t tm = gettime();
		tm_t date = gettime();

		uint64_t diff_time = difftime(&tm, &date);
		printk(_END "[0:%02u] "_END
					"- "_YELLOW
					"[%s] " _END "- " _GREEN "[INIT] " _CYAN "%s " _END "\n",
			   diff_time, part, name);

		if (irq_check_install(IRQ_PIT)) {
			kmsleep(20);
		}
	}
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                   KERNEL INIT                                  ||
// ! ||--------------------------------------------------------------------------------||

static int check_multiboot(uint32_t magic_number, uint32_t addr, uint32_t *kstack) {
	/**
	 * Init Multiboot
	 *
	 * - Check Magic Number
	 * - Check Multiboot Info
	 * - Check Multiboot flags
	 * - Initialize kernel stack
	 *
	 */
	if ((multiboot_init(magic_number, addr, kstack)) != 0) {
		__PANIC("Error: multiboot_init failed");
		__BSOD_UPDATE("Error: multiboot_init failed");
		bsod("MULTIBOOT INIT FAILED", __FILE__);
		return (1);
	} else {

		const char *m_device = multiboot_get_device_name();
		uint32_t m_mem_lower, m_mem_upper;
		uint32_t available_memory = get_available_memory(get_multiboot_info());

		m_mem_lower = multiboot_get_mem_lower();
		m_mem_upper = multiboot_get_mem_upper();

		printk("\t   - Device: " _GREEN "%s\n" _END, m_device);
		printk("\t   - Memory Lower: " _GREEN "%ld KB (%ld MB)\n" _END, m_mem_lower, m_mem_lower / 1024);
		printk("\t   - Memory Upper: " _GREEN "%ld KB (%ld MB)\n" _END, m_mem_upper, m_mem_upper / 1024);
		printk("\t   - Available Memory: " _GREEN "%ld KB (%ld MB)\n" _END, available_memory / 1024, available_memory / 1024 / 1024);
		printk("\t   - Kernel Stack: " _GREEN "%ld KB\n" _END, KERNEL_STACK_SIZE / 1024);

		kernel_log_info("LOG", "MULTIBOOT");
	}

	return (0);
}

static int init_system_components(void) {
	gdt_init();
	kernel_log_info("LOG", "GDT");

	tss_init(7, 0x10, 0x0);
	kernel_log_info("LOG", "TSS");

	idt_install();
	kernel_log_info("LOG", "IDT");
	isr_install();
	kernel_log_info("LOG", "ISR");
	irq_install();
	kernel_log_info("LOG", "IRQ");

	pit_setup();
	kernel_log_info("LOG", "PIT");

	if ((init_cpuid()) == true) {
		kernel_log_info("LOG", "CPUID");
		get_cpu_topology();
		kernel_log_info("LOG", "CPU TOPOLOGY");
	}
	// ps2_init();
	// kernel_log_info("LOG", "PS2");

	usb_init();
	kernel_log_info("LOG", "USB");

	keyboard_install();
	kernel_log_info("LOG", "KEYBOARD");

	if (fpu_is_present()) {
		enable_fpu();
		kernel_log_info("LOG", "FPU");
	} else {
		__WARND("FPU is not present");
	}

	rand_init();
	kernel_log_info("LOG", "RANDOM");

	return (0);
}

static int init_multitasking(void) {
	init_scheduler();
	kernel_log_info("LOG", "SCHEDULER");

	init_tasking();
	kernel_log_info("LOG", "TASKING");

	// thread_init();
	// kernel_log_info("LOG", "THREADS");
	return (0);
}

static int init_filesystems(uint32_t initrd_location, uint32_t initrd_end) {
	// Init devices
	devices_init();
	kernel_log_info("LOG", "DEVICES");

	if ((vfs_init()) != 0) {
		__WARND("Error: vfs_init failed, (Kernel will not use VFS)");
	} else {
		kernel_log_info("LOG", "VFS");
	}

	// Init initrd filesystem
	if (get_multiboot_info()->mods_count > 0) {
		if ((initrd_init(initrd_location, initrd_end)) != 0) {
			__PANIC("Error: initrd_init failed");
			__BSOD_UPDATE("Error: initrd_init failed");
			bsod("INITRD INIT FAILED", __FILE__);
			return (1);
		} else {
			kernel_log_info("LOG", "INITRD");
		}
	}
	initrd_display_hierarchy();

	/*
	**  IDE INIT
	**
	**  Integrated Drive Electronics initialization
	**  IDE Driver initialization
	*/

	if ((ide_init()) != 0) {
		__WARND("Error: ide_init failed, (Kernel will not use IDE Driver)");
	} else {
		/* Workflow IDE must be called after IDE init */
		// workflow_ide();
		kernel_log_info("LOG", "IDE");
	}

/**
 * FLOPPY INIT
 *
 * Floppy Driver initialization
 */
#if FLOPPY_DRIVER == 1
	// fdc_initialize();
	kernel_log_info("LOG", "FLOPPY");

	ksleep(1);

	// workflow_fdc();
	// kernel_log_info("LOG", "FLOPPY READ/WRITE");
#else
	__INFOD("FLOPPY Driver is disabled");
#endif

#if __TINYFS__ == 1
	Device *dev = device_get_by_id(0); // Get hda device
	TinyFS *tfs = NULL;

	/* Check if device is available */
	if (dev == NULL) {
		__WARND("Error: device_get_by_id failed, (Kernel will not use TINYFS)");
	} else {
		/* Init TINYFS */
		if ((tfs = tinyfs_init(dev)) == NULL) {
			__WARND("Error: tinyfs_init failed, (Kernel will not use TINYFS)");
		} else {
			/* Format TINYFS */
			if ((tinyfs_formater(tfs, false)) != 0) { // Hard format is disabled
				__WARND("Error: tinyfs_formater failed, (Kernel will not use TINYFS)");
			}
			/* Mount TINYFS on device */
			if ((vfs_mount(tfs->fs.vfs)) != 0) {
				__WARND("Error: vfs_mount failed, (Kernel will not use TINYFS)");
			} else {
				// workflow_tinyfs();
				kernel_log_info("LOG", "TINYFS");
			}
		}
	}
#else
	__INFOD("TINYFS is disabled");
#endif

	// kpause(); // Tmp, test hephaistos

	/*
	**  EXT2 INIT
	**
	**  EXT2 Filesystem initialization
	*/
#if __EXT2__ == 1
	// Todo:
	if ((ext2_init()) != 0) {
		__WARND("Error: vfs_create_fs failed, (Kernel will not use EXT2 Filesystem)");
	} else {
		kernel_log_info("LOG", "FILESYSTEM EXT2");
	}
	kpause();
#else
	__INFOD("EXT2 is disabled, (Kernel will not use EXT2 Filesystem)");
#endif
	return (0);
}

static int init_kernel(uint32_t magic_number, uint32_t addr, uint32_t *kstack) {
	tty_init();
	ksh_header();
	kernel_log_info("LOG", "TTY");
	time_init();
	kernel_log_info("LOG", "TIME");

	printk("       - "_YELLOW
		   "[LOG] " _END "- "_END _GREEN "[CHK] " _END " HHK: "_GREEN
		   "%s " _END "\n" _END,
		   __HIGHER_HALF_KERNEL__ == true ? "true" : "false");

	kernel_log_info("LOG", "TERMINAL");

	// Todo: Remove kerrno (unused / useless lib)
	// init_kerrno();
	// kernel_log_info("LOG", "KERRNO");

	check_multiboot(magic_number, addr, kstack);

	init_system_components();

	/**
	 * INIT INITRD
	 *
	 * Init initrd if multiboot modules are found
	 * Initrd -> initial ramdisk
	 */
	uint32_t initrd_location = 0;
	uint32_t initrd_end = 0;
	printk("Placement Addr: 0x%x\n", get_placement_addr());

	// Convertir les adresses physiques des modules en adresses virtuelles
	if (get_multiboot_info()->mods_count > 0) {
		// Conversion de mods_addr en adresse virtuelle
		uint32_t mods_addr_virtual = (uint32_t)(uintptr_t)get_multiboot_info()->mods_addr + KERNEL_VIRTUAL_BASE;

		// Récupérer l'adresse du premier module (initrd)
		initrd_location = *((uint32_t *)(uintptr_t)mods_addr_virtual); // Initrd start
		initrd_end = *(uintptr_t *)(mods_addr_virtual + 4);			   // Initrd end

		// Ajustement de l'adresse de placement
		printk("Placement Addr: 0x%x\n", initrd_end);
		set_placement_addr(initrd_end);

		printk("\t   - Initrd Location: " _GREEN);
		printk("0x%x\n" _END, initrd_location);
		printk("\t   - Initrd End: " _GREEN);
		printk("0x%x\n" _END, initrd_end);
	} else {
		__WARND("No multiboot modules found, kernel will not use initrd.");
	}

	printk("Kernel end addr: 0x%x\n", &_kernel_end);
	qemu_printf("Kernel end addr: 0x%x\n", &_kernel_end);

	if ((mmu_init()) != 0) {
		__PANIC("Error: mmu_init failed");
		__BSOD_UPDATE("Error: mmu_init failed");
		bsod("MMU INIT FAILED", __FILE__);
		return (1);
	}

	kernel_log_info("LOG", "PAGING");
	kernel_log_info("LOG", "HEAP");

	init_syscall();
	kernel_log_info("LOG", "SYSCALL");

	init_signals();
	kernel_log_info("LOG", "SIGNALS");

	// ksleep(1);
	// usb_test_write_and_read();

	init_filesystems(initrd_location, initrd_end);

	// Todo: Fix MMU clone directory
	// init_multitasking();

	return (0);
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                   KERNEL MAIN                                  ||
// ! ||--------------------------------------------------------------------------------||

int kmain(uint32_t magic_number, uint32_t addr, uint32_t *kstack) {
	ASM_CLI();
	if ((init_kernel(magic_number, addr, kstack)))
		return (1);
	if (__DISPLAY_INIT_LOG__) {
		printk("\n");
		printk(_RED);
		// terminal_write_n_char('#', VGA_WIDTH);
		printk(_END);
		printk("\n\n");
	}
	ASM_STI();

	tm_t date = gettime();
	printk("Date: " _GREEN "%04d-%02d-%d:%02d-%02d-%02d\n\n" _END, date.year + 2000, date.month, date.day, date.hours + 1, date.minutes, date.seconds);

#if __TEST_HEPHAISTOS__ == 1
	__INFOD(_GREEN "Test Hephaistos is enabled" _END);
	return hephaistos_workflow();
#else // !__TEST_HEPHAISTOS__
	__INFOD(_YELLOW "Test Hephaistos is disabled" _END);
#endif

	// 	pid_t test = fork();
	// 	if (test == 0) {
	// #include <cmds/cat.h>
	// #include <cmds/ls.h>
	// #include <cmds/mkdir.h>
	// #include <cmds/pwd.h>

	// 		pwd(0, NULL);
	// 		printk("TFS Current Node: %s\n", vfs_get_fs(TINYFS_FILESYSTEM_NAME)->nops->get_name(vfs_get_fs((TINYFS_FILESYSTEM_NAME))->fs_current_node));
	// 		ls(0, NULL);

	// 		cat(2, (char *[]){"cat", "file1.txt", NULL});

	// 		mkdir(2, (char *[]){"mkdir", "test", NULL});
	// 		ls(0, NULL);

	// 		sys_chdir("test");
	// 		printk("TFS Current Node: %s\n", vfs_get_fs(TINYFS_FILESYSTEM_NAME)->nops->get_name(vfs_get_fs((TINYFS_FILESYSTEM_NAME))->fs_current_node));

	// 		TinyFS_Inode *ino = tinyfs_get_inode(vfs_get_current_fs(), 0);
	// 		tinyfs_display_hierarchy(ino, 1);

	// 		pwd(0, NULL);
	// 		ls(0, NULL);

	// 		int fd = sys_creat("/test/test_file1.txt", O_RDWR);
	// 		int fd2 = sys_creat("test_file2.txt", O_RDWR);

	// 		ls(0, NULL);

	// 		if ((sys_write(fd, "Hello test File1 !\n", 20)) != 0) {
	// 			printk("Error: sys_write failed\n");
	// 		} else if ((sys_write(fd2, "Hello test File2 !\n", 20)) != 0) {
	// 			printk("Error: sys_write failed\n");
	// 		}

	// 		sys_close(fd);
	// 		sys_close(fd2);

	// 		cat(2, (char *[]){"cat", "test_file1.txt", NULL});
	// 		cat(2, (char *[]){"cat", "test_file2.txt", NULL});

	// 		ls(3, (char *[]){"ls", "-l", "/", NULL});

	// 		pause();
	// 	} else {
	// 		int status;
	// 		waitpid(test, &status, 0);
	// 		printk("Child process exited with status: %d\n", status);
	// 	}

	// kpause();

	kronos_shell();

	while (1) {
	}
	// Todo: Fix MMU clone directory
	pid_t pid = fork();
	if (pid == 0) {
		// Todo: Must enter in user space instead of using kernel space
		kronos_shell();
	} else {

		/*
		** Task 0 -> Kernel
		** Must infinite loop
		*/

		while (1) {
		}
	}
	return (0);
}

//     // Display initrd files

//     // list the contents of /

//     // Todo: KFS-6
//     // initrd_display_hierarchy();

//     // uint32_t esp;
//     // GET_ESP(esp);
//     // tss_init(7, 0x10, esp);
//     // kernel_log_info("LOG", "TSS");
//     // switch_to_user_mode();

//     // threads_test();
//     // process_test();

//     // uint32_t esp;

//     // GET_ESP(esp);
//     // printk("ESP: " _GREEN "0x%x\n" _END, esp);

//     // tss_set_stack_pointer(esp);
//     // tss_set_stack_segment(0x10);

//     // switch_to_user_mode();

//     // Todo: Fix priority
//     // task_set_priority(pid_tmp, TASK_PRIORITY_LOW);
//     // task_set_priority(pid_tmp2, TASK_PRIORITY_LOW);
//     // task_set_priority(pid_tmp3, TASK_PRIORITY_LOW);

//     // switch_to_user_mode();

//     Vfs *__tiny_vfs = vfs_get_fs(TINYFS_FILESYSTEM_NAME);
//     VfsNode *root = __tiny_vfs->fs_root;

//     // vfs_mkdir(__tiny_vfs, root, "/test", 0755);

//     __tiny_vfs->fops->mkdir(root, "test", 0755);
//     __tiny_vfs->fops->mkdir(root, "test2", 0755);
//     __tiny_vfs->fops->mkdir(root, "test3", 0755);
//     __tiny_vfs->fops->mkdir(root, "test4", 0755);

//     Dirent *dir = NULL;
//     uint32_t i = 0;

//     while ((dir = __tiny_vfs->fops->readdir(root, i)) != NULL) {
//         if (strcmp(dir->d_name, "test") == 0) {
//             VfsNode *newroot = vfs_finddir(__tiny_vfs, root, "test");
//             __tiny_vfs->fops->mkdir(newroot, "subtest", 0755);
//             __tiny_vfs->fops->create(newroot, "file.txt", 0755);
//         }
//         ++i;
//     }
//     tinyfs_display_hierarchy((TinyFS_Inode *)(root), 0);
//     // kpause();

//     printk("\n---------------------\n\n");

// #include <cmds/cd.h>
// #include <cmds/ls.h>
// #include <cmds/pwd.h>
// #include <syscall/chdir.h>
// #include <syscall/getcwd.h>

//     pwd(0, NULL);
//     ls(0, NULL);

//     if ((sys_chdir("/test")) != 0) {
//         printk("Error: sys_chdir failed\n");
//     }

//     printk("PWD: ");
//     pwd(0, NULL);
//     printk("Real PWD: %s\n", __tiny_vfs->nops->get_name(__tiny_vfs->fs_current_node));

//     // tinyfs_display_hierarchy((TinyFS_Inode *)(__tiny_vfs->fs_current_node), 0);
//     sys_chdir("/test/subtest");

//     printk("PWD: ");
//     pwd(0, NULL);
//     printk("Real PWD: %s\n", __tiny_vfs->nops->get_name(__tiny_vfs->fs_current_node));
//     kpause();

//     // pwd(0, NULL);
//     // ls(0, NULL);
//     // workflow_tinyfs();

//     kpause();

//     kfe(0, NULL);
//     printk("End of KFE\n");

//     pid_t pid2 = fork();
//     if (pid2 == 0) {

//         printk("ls: ");
//         ls(0, NULL);
//         ksleep(1);
//         printk("\npwd: ");
//         pwd(0, NULL);
//         ksleep(1);

//         cd(1, (char *[]){"cd", "test"}); // Todo: implement /... (ex: /test)
//         ksleep(1);

//         printk("ls: ");
//         ls(0, NULL);
//         ksleep(1);
//         printk("\npwd: ");
//         pwd(0, NULL);
//         ksleep(1);
//     } else {
//         int status;

//         waitpid(pid2, &status, 0);
//         printk("Child process exited with status: %d\n", status);
//     }
//     pause();

//     pid_t pid = fork();
//     if (pid == 0) {
//         // Todo: Must enter in user space instead of using kernel space
//         kronos_shell();
//     } else {

//         /*
//         ** Task 0 -> Kernel
//         ** Must infinite loop
//         */

//         while (1) {
//         }
//     }
//     return (0);
// }