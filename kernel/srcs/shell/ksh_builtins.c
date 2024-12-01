/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ksh_builtins.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/05 01:12:55 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/26 22:01:34 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <multiboot/multiboot.h>
#include <shell/builtins/builtins.h>
#include <shell/ksh.h>
#include <system/cpu.h>
#include <system/gdt.h>

#include <cmds/cat.h>
#include <cmds/cd.h>
#include <cmds/devices.h>
#include <cmds/disks.h>
#include <cmds/ls.h>
#include <cmds/ps.h>
#include <cmds/pwd.h>

#include <drivers/keyboard.h>

#include <workflows/workflows.h>

KshBuiltins __ksh_builtins[__BUILTINS_MAX];

static uint8_t __current_index = 0;

/**
 * Display the help menu for KSH builtins.
 */
static void __ksh_help(void) {
	printk("KSH builtins:\n");
	printk("- " _GREEN "help" _END ": display this help\n");
	printk("- " _GREEN "clear" _END ": clear the screen\n");
	printk("- " _GREEN "reboot" _END ": reboot kernel\n");
	printk("- " _GREEN "poweroff" _END "/" _GREEN "halt" _END "/" _GREEN "shutdown" _END ": shutdown kernel\n");
	printk("- " _GREEN "stack" _END "/" _GREEN "print-stack" _END ": display kernel stack (gdt)\n");
	printk("- " _GREEN "sections" _END ": display kernel sections\n");
	printk("- " _GREEN "mboot" _END "/" _GREEN "multiboot" _END ": display multiboot info\n");
	printk("- " _GREEN "kmmap" _END ": display kernel memory info\n");
	printk("- " _GREEN "setxkbmap" _END ": set keyboard layout\n");
	printk("- " _GREEN "cpuinfos" _END ": display cpu infos\n");
	printk("- " _GREEN "ps" _END ": display process infos\n");
	printk("- " _GREEN "cat" _END ": display file content\n");
	printk("- " _GREEN "pwd" _END ": display current directory\n");
	printk("- " _GREEN "disks" _END ": display disks infos\n");
	printk("- " _GREEN "devices" _END ": display devices infos\n");
	printk("- " _GREEN "ls" _END ": display files in current directory\n");
	printk("- " _GREEN "cd" _END ": change directory\n");
}

/**
 * Add a builtin command to the list of available builtins.
 *
 * @param names An array of strings representing the names of the builtin command.
 * @param fn    A pointer to the function implementing the builtin command.
 */
static void __add_builtin(char *names[__BUILTINS_MAX_NAMES], void *fn) {
	for (uint8_t i = 0; names[i] && i < __BUILTINS_MAX_NAMES; i++) {
		if (names[i][0] == 0)
			break;
		strncpy(__ksh_builtins[__current_index].names[i], names[i], sizeof(__ksh_builtins[__current_index].names[i]) - 1);
		__ksh_builtins[__current_index].names[i][sizeof(__ksh_builtins[__current_index].names[i]) - 1] = '\0';
	}
	if (__current_index >= __BUILTINS_MAX) {
		printk("       [ERROR] Too many builtins\n");
		return;
	}
	__ksh_builtins[__current_index].function = fn;
	++__current_index;
}

/**
 * Initialize the list of KSH builtins.
 */
void __ksh_init_builtins(void) {
	memset(__ksh_builtins, 0, sizeof(__ksh_builtins));

	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"clear", ""}, &ksh_clear);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"poweroff", "halt", "shutdown", ""}, &poweroff);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"reboot", ""}, &reboot);
	// __add_builtin((char *[__BUILTINS_MAX_NAMES]){"print-stack", "stack", ""}, &print_stack);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"print-gdt", "gdt", ""}, &print_gdt);
	// __add_builtin((char *[__BUILTINS_MAX_NAMES]){"gdt-test", ""}, &gdt_test);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"time", ""}, &pit_display_phase);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"mboot", "multiboot", ""}, &print_multiboot_info);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"help", ""}, &__ksh_help);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"kmmap", ""}, &print_memory_map);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"setxkbmap", ""}, &setxkbmap);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"cpuinfos", ""}, &get_cpu_informations);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"ps", ""}, &ps);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"cat", ""}, &cat);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"pwd", ""}, &pwd);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"disks", ""}, &disks);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"devices", ""}, &__devices);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"ls", ""}, &ls);
	__add_builtin((char *[__BUILTINS_MAX_NAMES]){"cd", ""}, &cd);
}

/**
 * Execute the specified builtin command.
 *
 * @param argc The number of arguments.
 * @param argv An array of strings representing the arguments.
 */
void __ksh_execute_builtins(int argc, char **argv) {
	if (argc == 0)
		return;

	for (uint8_t i = 0; i < __BUILTINS_MAX; i++) {
		for (uint8_t j = 0; j < __BUILTINS_MAX_NAMES; j++) {
			if (__ksh_builtins[i].names[j][0] == '\0')
				continue;
			if (strcmp(argv[0], __ksh_builtins[i].names[j]) == 0) {
				if (__ksh_builtins[i].function != NULL) {
					__ksh_builtins[i].function(argc, argv);
					return;
				} else {
					printk("       Command not implemented: %s\n", argv[0]);
					return;
				}
			}
		}
	}
	printk("       Unknown command: %s\n", argv[0]);
}