/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kshell.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 14:40:07 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/06/27 13:10:32 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef KSHELL_H
#define KSHELL_H

#include "../kernel.h"
#include "../terminal.h"

#include "../system/pit.h"

#define __PROMPT__ COLOR_END "KSH" COLOR_GREEN " $> " COLOR_END
#define __PROMPT_LEN__ (sizeof(__PROMPT__) - 1)
#define __PROMPT_ASCII_LEN__ (size_t)(7)

#define __HEADER_WIDTH__ (size_t)(VGA_WIDTH)
#define __HEADER_HEIGHT__ (size_t)(11)

#define DISPLAY_PROMPT() kprintf(__PROMPT__)
#define KSH_CHAR(x, y) kshell_buffer[(y) * VGA_WIDTH + (x)]

extern void kronos_shell(void);

/*
KSHELL BUFFER =
Save chaque lignes après la touche entrée
comme ça, on peut scroll facilement
*/
extern uint16_t *kshell_buffer;

extern size_t kshell_max_line;         // Max line of the shell
extern size_t kshell_current_max_line; // Current max line of the shell
extern size_t kshell_min_line;         // Min line of the shell
extern size_t kshell_current_line;     // Current line of the shell

/*******************************************************************************
 *                                    UTILS                                    *
 ******************************************************************************/

extern void kshell_init(void);

extern size_t kshell_get_last_character_index(void);
extern void kshell_del_char_location(size_t x, size_t y);
extern void kshell_insert_char_location(char c, size_t x, size_t y);
extern void ksh_save_line(size_t y);

/*******************************************************************************
 *                              INLINE FUNCTIONS                               *
 ******************************************************************************/

#endif // !KSHELL_H