/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmuf.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/17 14:39:48 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:45:59 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MMUF_H
#define MMUF_H

#include <mm/mmu.h>
#include <stdint.h>

// Size of a frame (4KB)
#define FRAME_SIZE 4096

// Align an address to the nearest lower frame boundary
#define FRAME_ALIGN(addr) ((addr) & ~(FRAME_SIZE - 1))

// Maximum number of frames (4GB / 4KB = 1048576)
#define MAX_FRAMES 1048576

// Possible states of a frame
typedef enum {
	FRAME_FREE = 0,
	FRAME_USED = 1,
	FRAME_RESERVED = 2,
	FRAME_KERNEL = 3
} frame_state_t;

// Bit definitions
#define BITS_PER_BYTE 8
#define BITS_PER_UINT32 (sizeof(uint32_t) * BITS_PER_BYTE)				  // 32 bits
#define BITMAP_SIZE(mem_size) ((mem_size) / FRAME_SIZE / BITS_PER_UINT32) // Number of uint32_t needed

// Flags for pages
#define PAGE_PRESENT 0x1  // Page present in memory
#define PAGE_RW 0x2		  // Read-write
#define PAGE_USER 0x4	  // User mode
#define PAGE_ACCESSED 0x8 // Page accessed
#define PAGE_DIRTY 0x10	  // Page modified
#define PAGE_NX 0x20	  // No Execute

// Function prototypes
void init_frames(uint32_t mem_size);
uint32_t allocate_frame(page_t *page, int is_kernel, int flags);
void free_frame(page_t *page);
uint32_t get_frame_count(void);
uint32_t get_used_frames(void);
void frame_mark_kernel(uint32_t start_frame, uint32_t end_frame);
void frame_mark_reserved(uint32_t start_frame, uint32_t end_frame);

#endif /* !MMUF_H */