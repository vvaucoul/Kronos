/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot_mmap.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/29 11:33:34 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/26 21:04:04 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <multiboot/multiboot.h>
#include <multiboot/multiboot_mmap.h>

#include <mm/mm.h>

#include <kernel.h>
#include <system/pit.h>

/**
 * @brief Retrieves the total amount of available memory from the multiboot information structure.
 *
 * This function parses the memory map provided by the multiboot information structure
 * and calculates the total amount of available memory.
 *
 * @param mb_info_ptr Pointer to the multiboot information structure.
 * @return The total amount of available memory in bytes. Returns 0 if no memory map is provided.
 */
uint32_t get_available_memory(void *mb_info_ptr) {
	multiboot_info_t *mb_info = (multiboot_info_t *)mb_info_ptr;

	if (!(mb_info->flags & MULTIBOOT_FLAG_MMAP)) {
		__WARN("No memory map provided by multiboot\n", 0);
		return 0;
	}

	// Conversion de mmap_addr en adresse virtuelle
	multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)PHYS_TO_VIRT(mb_info->mmap_addr);
	uint32_t mmap_end = mb_info->mmap_addr + mb_info->mmap_length;

	uint32_t available_memory = 0;

	// Boucle sur chaque entrée dans la mmap
	while ((uintptr_t)mmap < (uintptr_t)PHYS_TO_VIRT(mmap_end)) {
		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) { // Mémoire utilisable
			available_memory += mmap->len;
		}
		mmap = (multiboot_mmap_entry_t *)((uintptr_t)mmap + mmap->size + sizeof(mmap->size));
	}

	return available_memory;
}