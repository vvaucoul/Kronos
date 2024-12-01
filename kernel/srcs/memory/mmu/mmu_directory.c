/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmu_directory.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/23 18:35:09 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/11/15 19:57:15 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/mm.h>
#include <mm/mmu.h>

page_directory_t *kernel_directory __attribute__((aligned(4096))) = NULL;
page_directory_t *current_directory __attribute__((aligned(4096))) = NULL;

/**
 * Retrieves the current page directory.
 *
 * @return A pointer to the current page directory.
 */
page_directory_t *mmu_get_current_directory(void) {
	return (current_directory);
}

/**
 * Retrieves the kernel page directory.
 *
 * @return A pointer to the kernel page directory.
 */
page_directory_t *mmu_get_kernel_directory(void) {
	return (kernel_directory);
}

/**
 * Sets the current page directory for the MMU.
 *
 * @param dir The page directory to set as the current directory.
 */
void mmu_set_current_directory(page_directory_t *dir) {
	if (dir == NULL) return;
	current_directory = dir;
}

/**
 * @brief Switches the current page directory to the specified directory.
 *
 * This function is responsible for switching the current page directory to the
 * specified directory. It updates the page directory register (CR3) with the
 * physical address of the new directory.
 *
 * @param dir The page directory to switch to.
 */
void mmu_switch_page_directory(page_directory_t *dir) {
	if (dir == NULL) return;
	mmu_set_current_directory(dir);
	switch_page_directory(dir->physicalAddr); // Pass as uint32_t
	mmu_flush_tlb();
}

/**
 * @brief Creates a new page directory.
 *
 * This function allocates memory for a new page directory and initializes it.
 * It copies the kernel page directory to the new page directory and assigns
 * the physical address of the new page directory.
 *
 * @return A pointer to the newly created page directory, or NULL if the
 * allocation fails.
 */
page_directory_t *mmu_create_page_directory(void) {
	// Allocate memory for the new page directory with physical address
	uint32_t new_dir_phys;
	page_directory_t *new_dir = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), &new_dir_phys);
	if (new_dir == NULL || new_dir_phys == 0) {
		return (NULL);
	}
	memset(new_dir, 0, sizeof(page_directory_t));

	// Assign the physical address of the new page directory
	new_dir->physicalAddr = new_dir_phys;

	// Copy the kernel page directory to the new page directory
	memcpy(new_dir, kernel_directory, sizeof(page_directory_t));

	return (new_dir);
}