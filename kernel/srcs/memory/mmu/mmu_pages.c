/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmu_pages.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/23 18:36:47 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/11/15 19:57:24 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/mm.h>
#include <mm/mmu.h>
#include <mm/mmuf.h>
#include <mm/pt_pool.h>

/**
 * Retrieves the page corresponding to the given address from the specified page directory.
 *
 * @param address The address for which to retrieve the page.
 * @param dir The page directory from which to retrieve the page.
 * @return A pointer to the page structure corresponding to the given address.
 */
page_t *mmu_get_page(uint32_t address, page_directory_t *dir) {
	if (dir == NULL) return NULL;

	uint32_t table_idx = address / (PAGE_SIZE * PAGE_ENTRIES); // address / 0x400000
	uint32_t page_idx = (address / PAGE_SIZE) % PAGE_ENTRIES;  // (address / 0x1000) % 1024
	if (dir->tables[table_idx]) {
		return &dir->tables[table_idx]->pages[page_idx];
	} else {
		return NULL;
	}
}

/**
 * @brief Creates a page and maps it in the page directory.
 *
 * @param address The virtual address to map.
 * @param dir The page directory.
 * @param is_kernel Flag indicating if the page is for kernel space.
 * @return A pointer to the created page, or NULL on failure.
 */
page_t *mmu_create_page(uint32_t address, page_directory_t *dir, int is_kernel) {
	if (dir == NULL) return NULL;

	uint32_t page_idx = address / PAGE_SIZE;
	uint32_t table_idx = page_idx / PAGE_ENTRIES;

	if (!dir->tables[table_idx]) {
		page_table_t *new_table = pagetable_pool_alloc();
		if (!new_table) {
			return NULL;
		}

		memset(new_table, 0, sizeof(page_table_t));
		dir->tables[table_idx] = new_table;

		uint32_t table_phys = mmu_get_physical_address((void *)new_table);
		if (table_phys == 0) {
			dir->tables[table_idx] = NULL;
			return NULL;
		}

		dir->tablesPhysical[table_idx] = table_phys | PAGE_PRESENT | PAGE_RW;
		if (!is_kernel) {
			dir->tablesPhysical[table_idx] |= PAGE_USER;
		}
	}

	return &dir->tables[table_idx]->pages[page_idx % PAGE_ENTRIES];
}

/**
 * @brief Destroys a page in the memory management unit (MMU).
 *
 * This function is responsible for destroying a page in the MMU given its address and the page directory.
 *
 * @param address The address of the page to be destroyed.
 * @param dir The page directory containing the page.
 */
void mmu_destroy_page(uint32_t address, page_directory_t *dir) {
	if (dir == NULL) return;

	address /= PAGE_SIZE;
	uint32_t table_idx = address / PAGE_ENTRIES;
	if (dir->tables[table_idx]) {
		dir->tables[table_idx]->pages[address % PAGE_ENTRIES].frame = 0;
		dir->tables[table_idx]->pages[address % PAGE_ENTRIES].present = 0;
	}
}

/**
 * @brief Destroy a page directory.
 *
 * This function is responsible for destroying a page directory.
 *
 * @param dir The page directory to be destroyed.
 */
void mmu_destroy_page_directory(page_directory_t *dir) {
	if (dir == NULL) return;

	for (int i = 0; i < PAGE_ENTRIES; i++) {
		if (dir->tables[i]) {
			for (int j = 0; j < PAGE_ENTRIES; j++) {
				free_frame(&dir->tables[i]->pages[j]);
			}
			kfree(dir->tables[i]);
		}
	}
	kfree(dir);
}