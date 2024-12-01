/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot_mmap.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/29 11:33:46 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/26 21:04:19 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MULTIBOOT_MMAP_H
#define MULTIBOOT_MMAP_H

#include <stdint.h>

#define KERNEL_STACK_MARKER 0x4B52304E // KRONOST (KRONOS STACK)
#define MAX_MEMORY_SECTIONS 10

#define MULTIBOOT_MEMORY_AVAILABLE 1

uint32_t get_available_memory(void *mb_info_ptr);

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

#endif /* !MULTIBOOT_MMAP_H */