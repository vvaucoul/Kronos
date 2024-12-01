/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ksh.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 14:40:02 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/23 23:13:41 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <drivers/keyboard.h>
#include <shell/ksh.h>
#include <shell/ksh_args.h>
#include <shell/ksh_builtins.h>
#include <shell/ksh_termcaps.h>

#include <asm/asm.h>
#include <mm/mm.h>

#include <multitasking/process.h>

uint32_t ksh_max_line = VGA_HEIGHT - __HEADER_HEIGHT__;
uint32_t ksh_min_line = __HEADER_HEIGHT__;
uint32_t ksh_current_line = __HEADER_HEIGHT__;
uint32_t ksh_current_max_line = __HEADER_HEIGHT__;

uint32_t *ksh_buffer = (uint32_t *)0x0000B000;

bool __ksh_is_running = false;

bool ksh_is_running(void) {
	return (__ksh_is_running);
}

void ksh_clear(void) {
	KSH_CLR_TERM_SH();
	tty_clear_screen();
	ksh_current_line = __HEADER_HEIGHT__;
	ksh_current_max_line = __HEADER_HEIGHT__;
	ksh_min_line = __HEADER_HEIGHT__;
}

void ksh_execute_command(void) {
	/* SIMPLE COMMAND EXECUTOR */
	char __formated_command[128];

	bzero(__formated_command, 128);
	strclr(__formated_command, ksh_line_buffer);
	if (__formated_command[0] != 0) {
		char **argv = ksh_parse_args(__formated_command);
		int argc = ksh_get_argc(argv);

		__ksh_execute_builtins(argc, argv);
		ksh_add_line_history(__formated_command);
		kfree(argv);
	}
	ksh_buffer_clear();
	tty_column = 0;
	DISPLAY_PROMPT();
	tty_column = __PROMPT_ASCII_LEN__;
	// update_cursor();
}

void ksh_handle_input(char c) {
	switch (c) {
		case '\n':
			ksh_write_char(c);
			ksh_execute_command();
			break;
		case '\b': ksh_del_one(); break;
		case 0x1b: poweroff(); break;
		default: ksh_write_char(c); break;
	}
}

void kronos_shell(void) {
	ASM_CLI();
	ksh_init();
	__ksh_init_builtins();
	printk("\nCreate Shell at "_GREEN
		   "[0x%x]"_END
		   " with PID: "_GREEN
		   "[%d]"_END
		   "\n",
		   ksh_buffer, getpid());
	ASM_STI();
	printk("Welcome to " _RED "KSH" _END " !\n");
	DISPLAY_PROMPT();
	// update_cursor();
	__ksh_is_running = true;

	while (1) {
		char c = getchar();
		ksh_handle_input(c);
	}
}

#undef __PROMPT__
#undef __PROMPT_LEN__
#undef __PROMPT_ASCII_LEN__
#undef __HEADER_WIDTH__
#undef __HEADER_HEIGHT__

#undef KSH_CLR_TERM_SH
#undef KSH_UPDATE_TERM_SH