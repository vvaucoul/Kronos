/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ksh_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/24 15:41:33 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 11:07:18 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <shell/ksh.h>

#include <ctype.h>

size_t ksh_get_last_character_index(void) {
	// // TODO Debug

	// size_t i = VGA_WIDTH - 1;
	// while (i > __PROMPT_ASCII_LEN__ && isprint(TERMINAL_CHAR(terminal_column, terminal_row)) == true && TERMINAL_CHAR(terminal_column, terminal_row) == ' ') {
	// 	i--;
	// }
	// return (i);
}

static void ksh_move_offset_line_right(size_t x, size_t y, size_t offset) {
	// for (size_t i = VGA_WIDTH - 2; i > __PROMPT_ASCII_LEN__ && i > x; i--) {
	// 	TERMINAL_CHAR(i, y) = TERMINAL_CHAR(i - offset, y);
	// }
}

static void ksh_move_offset_line_left(size_t x, size_t y, size_t offset) {
	// for (size_t i = x; i < VGA_WIDTH - 1; i++) {
	// 	TERMINAL_CHAR(i, y) = TERMINAL_CHAR(i + offset, y);
	// }
}

void ksh_save_line(size_t y) {
	printk("\n");
	// for (size_t x = __PROMPT_ASCII_LEN__; x < VGA_WIDTH - 1; x++) {
	// 	KSH_CHAR(x, y - __HEADER_HEIGHT__) = TERMINAL_CHAR(x, y);
	// 	KSH_CHAR(x + 1, y - __HEADER_HEIGHT__) = 0;
	// }

	// return ;
	// display save
	char buffer[VGA_WIDTH];
	memset(buffer, 0, VGA_WIDTH);

	for (size_t x = 0; x < VGA_WIDTH; x++) {
		uint16_t value = KSH_CHAR(x, y - __HEADER_HEIGHT__);
		// printk("%x", value);
		char lo = value & 0xFF;
		char hi = (value >> 8);
		char c = lo | hi >> 8;
		buffer[x] = c;
	}
	printk("Line: %s\n", buffer);
}

void ksh_del_char_location(size_t x, size_t y) {
	ksh_move_offset_line_left(x, y, 1);
	// update_cursor();
}

void ksh_insert_char_location(char c, size_t x, size_t y) {
	ksh_move_offset_line_right(x, y, 1);
	putchar(c);
	// update_cursor();
}