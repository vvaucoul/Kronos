/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmuf.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/17 14:39:30 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/11/15 19:57:21 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/mm.h>
#include <mm/mmuf.h>

#include <mm/ealloc.h>
#include <system/panic.h>

#include <multiboot/multiboot.h>
#include <multiboot/multiboot_mmap.h>

#include <string.h> // For memset
#include <system/serial.h>

// Frame bitmap
static uint32_t *frame_bitmap = NULL;

// Total number of frames
static uint32_t frame_count = 0;

// Number of used and free frames
static uint32_t used_frames = 0;
static uint32_t free_frames = 0;

/**
 * @brief Sets a frame as used in the bitmap.
 *
 * @param frame_num The number of the frame to set.
 */
static void set_frame(uint32_t frame_num) {
	frame_bitmap[frame_num / BITS_PER_UINT32] |= (1 << (frame_num % BITS_PER_UINT32));
	used_frames++;
	free_frames--;
}

/**
 * @brief Sets a frame as free in the bitmap.
 *
 * @param frame_num The number of the frame to set.
 */
static void clear_frame(uint32_t frame_num) {
	frame_bitmap[frame_num / BITS_PER_UINT32] &= ~(1 << (frame_num % BITS_PER_UINT32));
	used_frames--;
	free_frames++;
}

/**
 * @brief Tests if a frame is used.
 *
 * @param frame_num The number of the frame to test.
 * @return 1 if the frame is used, 0 otherwise.
 */
static uint32_t test_frame(uint32_t frame_num) {
	return (frame_bitmap[frame_num / BITS_PER_UINT32] & (1 << (frame_num % BITS_PER_UINT32))) != 0;
}

/**
 * @brief Finds the first free frame.
 *
 * @return The number of the first free frame, or calls __PANIC if no free frame is available.
 */
static uint32_t first_frame(void) {
	for (uint32_t i = 0; i < frame_count; i++) {
		if (!test_frame(i)) {
			return i;
		}
	}
	__PANIC("first_frame: No free frames!");
	return 0; // Not reached
}

/**
 * @brief Allocates a frame for a given page.
 *
 * @param page Pointer to the page structure.
 * @param is_kernel Indicates if the frame is for kernel space.
 * @param flags Page flags (PAGE_PRESENT, PAGE_RW, etc.).
 * @return The number of the allocated frame.
 */
uint32_t allocate_frame(page_t *page, int is_kernel, int flags) {
	if (page->frame != 0) {
		return page->frame;
	} else {
		uint32_t frame = first_frame();
		set_frame(frame);
		page->frame = frame;

		// Assign flags to the page
		page->present = (flags & PAGE_PRESENT) ? 1 : 0;
		page->rw = (flags & PAGE_RW) ? 1 : 0;
		page->user = (flags & PAGE_USER) ? 1 : 0;
		page->accessed = (flags & PAGE_ACCESSED) ? 1 : 0;
		page->dirty = (flags & PAGE_DIRTY) ? 1 : 0;
		page->nx = (flags & PAGE_NX) ? 1 : 0;

		return frame;
	}
}

/**
 * @brief Frees a memory frame.
 *
 * @param page Pointer to the page structure.
 */
void free_frame(page_t *page) {
	if (page->frame == 0) {
		return; // The frame is already free
	} else {
		clear_frame(page->frame);
		page->frame = 0;
	}
}

/**
 * @brief Gets the total number of frames.
 *
 * @return The total number of frames.
 */
uint32_t get_frame_count(void) {
	return frame_count;
}

/**
 * @brief Gets the number of used frames.
 *
 * @return The number of used frames.
 */
uint32_t get_used_frames(void) {
	return used_frames;
}

/**
 * @brief Marks a range of frames as used by the kernel.
 *
 * @param start_frame The number of the first frame to mark.
 * @param end_frame The number of the last frame to mark.
 */
void frame_mark_kernel(uint32_t start_frame, uint32_t end_frame) {
	for (uint32_t frame = start_frame; frame < end_frame; frame++) {
		if (!test_frame(frame)) {
			set_frame(frame);
			// Optional: add a specific state if necessary
		}
	}
}

/**
 * @brief Marks a range of frames as reserved.
 *
 * @param start_frame The number of the first frame to mark.
 * @param end_frame The number of the last frame to mark.
 */
void frame_mark_reserved(uint32_t start_frame, uint32_t end_frame) {
	for (uint32_t frame = start_frame; frame < end_frame; frame++) {
		if (!test_frame(frame)) {
			set_frame(frame);
			// Optional: add a specific state if necessary
		}
	}
}

/**
 * @brief Initializes the frames for memory paging.
 *
 * This function initializes the frames used for memory paging.
 * It sets up the frame bitmap, marks the frames used by the kernel and the bitmap itself,
 * and initializes the used and free frame counters.
 *
 * @param mem_size The total memory size in bytes.
 */
void init_frames(uint32_t mem_size) {
	// Get Multiboot information
	multiboot_info_t *mb_info = get_multiboot_info();
	if (!(mb_info->flags & MULTIBOOT_FLAG_MMAP)) {
		__PANIC("init_frames: No memory map provided by bootloader!");
	}

	// Initialize the total number of frames
	frame_count = mem_size / FRAME_SIZE;
	if (frame_count > MAX_FRAMES) {
		frame_count = MAX_FRAMES;
	}

	// Calculate the size of the bitmap (in uint32_t)
	uint32_t bitmap_size = (frame_count / BITS_PER_UINT32) + 1;

	// Allocate the frame bitmap
	frame_bitmap = (uint32_t *)(ealloc_aligned(bitmap_size * sizeof(uint32_t), sizeof(uint32_t)));
	qemu_printf("\t   - Frame Allocator: Frame Bitmap at 0x%x\n", frame_bitmap);

	// Initialize bitmap with all frames FREE
	memset_s(frame_bitmap, bitmap_size * sizeof(uint32_t), 0x00, bitmap_size * sizeof(uint32_t));
	used_frames = 0;
	free_frames = frame_count;

	// Mark the lower 1MB as reserved (BIOS, VGA, etc.)
	frame_mark_reserved(0, 256); // First 1MB (256 frames)

	// Mark kernel frames
	extern uint32_t _kernel_start;
	extern uint32_t _kernel_end;
	uint32_t kernel_start_frame = FRAME_ALIGN((uintptr_t)&_kernel_start - KERNEL_VIRTUAL_BASE) / FRAME_SIZE;
	uint32_t kernel_end_frame = FRAME_ALIGN((uintptr_t)&_kernel_end - KERNEL_VIRTUAL_BASE) / FRAME_SIZE;
	frame_mark_kernel(kernel_start_frame, kernel_end_frame);

	// Mark bitmap frames as reserved
	uint32_t bitmap_frames = (bitmap_size * sizeof(uint32_t) + FRAME_SIZE - 1) / FRAME_SIZE;
	uint32_t bitmap_start_frame = FRAME_ALIGN((uintptr_t)frame_bitmap - KERNEL_VIRTUAL_BASE) / FRAME_SIZE;
	frame_mark_reserved(bitmap_start_frame, bitmap_start_frame + bitmap_frames);

	// Traverse the memory map and mark available frames
	uintptr_t mmap_addr = (uintptr_t)mb_info->mmap_addr + KERNEL_VIRTUAL_BASE;
	uintptr_t mmap_end = mmap_addr + mb_info->mmap_length;
	multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mmap_addr;

	while ((uintptr_t)mmap < mmap_end) {
		// Multiboot MMap entries start with a size field
		uint32_t entry_size = mmap->size;
		multiboot_mmap_entry_t *entry = mmap;

		// Check if the entry is of type available
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			uint64_t start = entry->addr;
			uint64_t end = entry->addr + entry->len;

			// Ignore addresses above the managed memory
			if (start >= mem_size) {
				mmap = (multiboot_mmap_entry_t *)((uintptr_t)mmap + entry_size + sizeof(entry->size));
				continue;
			}
			if (end > mem_size) {
				end = mem_size;
			}

			// Calculate frame numbers
			uint32_t start_frame = start / FRAME_SIZE;
			uint32_t end_frame = end / FRAME_SIZE;

			// Mark available frames as free
			for (uint32_t frame = start_frame; frame < end_frame && frame < frame_count; frame++) {
				clear_frame(frame);
			}
			qemu_printf("\t   - Frame Allocator: Available Memory: 0x%llx - 0x%llx: %lld Mo\n", start, end, (end - start) / 1024 / 1024);
		}

		// Move to the next entry
		mmap = (multiboot_mmap_entry_t *)((uintptr_t)mmap + entry_size + sizeof(entry->size));
	}

	qemu_printf("\t   - Frame Allocator: Total Frames: "
				"%lld"
				"\n\t   - Used Frames: "
				"%lld"
				"\n\t   - Free Frames: "
				"%lld"
				"\n",
				frame_count, used_frames, free_frames);
	qemu_printf("\t   - Frame Allocator start addr: 0x%x - end addr: 0x%x\n", frame_bitmap, frame_bitmap + (bitmap_size * sizeof(uint32_t)));
}
