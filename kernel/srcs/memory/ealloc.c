/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ealloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/31 18:09:05 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:24:01 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/ealloc.h>
#include <mm/mm.h>
#include <multiboot/multiboot_mmap.h>
#include <stddef.h>
#include <string.h>

#include <system/serial.h>

// Initial placement address, typically set to the end of the kernel section
static uint32_t placement_addr = ((uint32_t)(&_kernel_end) - KERNEL_VIRTUAL_BASE);

static void *ealloc_internal(uint32_t size, uint32_t align_size, uint32_t *phys_addr) {
	// Align the placement address if necessary
	if (align_size && (placement_addr & (align_size - 1))) {
		placement_addr = (placement_addr + align_size) & ~(align_size - 1);
	}

	uint32_t new_addr = placement_addr + size;

	// Check for overflow
	if (new_addr < placement_addr) {
		return NULL;
	}

	// Store the physical address if requested
	if (phys_addr) {
		*phys_addr = placement_addr;
	}

	// Convert the physical address to a virtual address
	void *addr = (void *)(uintptr_t)(placement_addr + KERNEL_VIRTUAL_BASE);

	// Update the placement address
	placement_addr = new_addr;

	return addr;
}

/**
 * @brief Allocates a block of memory of the given size.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void *ealloc(uint32_t size) {
	return ealloc_internal(size, 0, NULL);
}

/**
 * @brief Allocates a block of memory of the given size with the specified alignment.
 *
 * @param size The size of the memory block to allocate.
 * @param align_size The alignment size.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void *ealloc_aligned(uint32_t size, uint32_t align_size) {
	return ealloc_internal(size, align_size, NULL);
}

/**
 * @brief Allocates a block of memory of the given size with the specified alignment and returns its physical address.
 *
 * @param size The size of the memory block to allocate.
 * @param align_size The alignment size.
 * @param phys A pointer to store the physical address of the allocated memory block.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void *ealloc_aligned_physic(uint32_t size, uint32_t align_size, uint32_t *phys) {
	return ealloc_internal(size, align_size, phys);
}

/**
 * @brief Allocates a block of memory of the given size and initializes it to zero.
 *
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void *ecalloc(uint32_t size) {
	void *ptr = ealloc(size);
	if (ptr) {
		memset(ptr, 0, size);
	}
	return ptr;
}

/**
 * @brief Inserts data from a specified address into a given memory block.
 *
 * @param ptr The pointer to the destination memory block.
 * @param addr The source address of the data to insert.
 * @param size The size of the data to insert.
 */
void einsert(void *ptr, uint32_t addr, uint32_t size) {
	memcpy(ptr, (void *)(addr + KERNEL_VIRTUAL_BASE), size);
}

/**
 * @brief Gets the current placement address.
 *
 * @return The current placement address.
 */
uint32_t get_placement_addr(void) {
	return placement_addr;
}

/**
 * @brief Sets the placement address to a specified value.
 *
 * @param addr The new placement address.
 */
void set_placement_addr(uint32_t addr) {
	placement_addr = addr;
}
