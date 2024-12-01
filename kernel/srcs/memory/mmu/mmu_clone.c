/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmu_clone.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 23:24:34 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 10:38:12 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/kheap.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <mm/mmuf.h>
#include <mm/pt_pool.h>
#include <system/serial.h>

/**
 * @brief Clone un répertoire de pages existant.
 *
 * @param src Répertoire de pages source à cloner.
 * @return Un pointeur vers le nouveau répertoire de pages cloné, ou NULL en cas d'échec.
 */
page_directory_t *mmu_clone_page_directory(page_directory_t *src) {
	// Allocate memory for the new page directory with physical address
	uint32_t new_dir_phys;
	page_directory_t *new_dir = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), &new_dir_phys);
	if (new_dir == NULL || new_dir_phys == 0) {
		qemu_printf("mmu_clone_page_directory: Failed to allocate memory for the new directory\n");
		return NULL;
	}
	memset(new_dir, 0, sizeof(page_directory_t));

	// Assign the physical address of the new page directory
	new_dir->physicalAddr = new_dir_phys;
	qemu_printf("Cloned page directory: VA=%p, PA=0x%X\n", (void *)new_dir, new_dir_phys);

	// Clone each page table
	for (int i = 0; i < PAGE_ENTRIES; i++) {
		if (src->tables[i]) {
			// Allocate a new page table with physical address
			uint32_t new_table_phys;
			page_table_t *new_table = (page_table_t *)kmalloc_ap(sizeof(page_table_t), &new_table_phys);
			if (!new_table || new_table_phys == 0) {
				qemu_printf("mmu_clone_page_directory: Failed to allocate page table for index %d.\n", i);
				mmu_destroy_page_directory(new_dir);
				return NULL;
			}
			memset(new_table, 0, sizeof(page_table_t));
			qemu_printf("Allocated new page table for index %d: VA=%p, PA=0x%X\n", i, (void *)new_table, new_table_phys);

			// Copy the page table entries
			for (int j = 0; j < PAGE_ENTRIES; j++) {
				if (src->tables[i]->pages[j].frame != 0) {
					new_table->pages[j] = src->tables[i]->pages[j];

					// Allocate a new frame for the page if needed
					if (!allocate_frame(&new_table->pages[j], 1, PAGE_PRESENT | PAGE_RW)) {
						qemu_printf("mmu_clone_page_directory: Failed to allocate frame for page [%d][%d].\n", i, j);
						mmu_destroy_page_directory(new_dir);
						return NULL;
					}

					qemu_printf("Cloning page [%d][%d]: Original frame=0x%X, new frame=0x%X\n",
								i, j, src->tables[i]->pages[j].frame, new_table->pages[j].frame);
				} else {
					qemu_printf("Cloning page [%d][%d]: No frame to allocate, skipping.\n", i, j);
				}
			}

			// Assign the new page table to the new directory
			new_dir->tables[i] = new_table;
			new_dir->tablesPhysical[i] = new_table_phys | PAGE_PRESENT | PAGE_RW;
			qemu_printf("Mapped new page table in directory at index %d: PA=0x%X\n", i, new_dir->tablesPhysical[i]);
		}
	}

	qemu_printf("Page directory cloning completed successfully.\n");
	return new_dir;
}
