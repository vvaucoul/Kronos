/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmu_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 23:53:14 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/30 10:25:06 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/ealloc.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <mm/mmuf.h>
#include <mm/pt_pool.h>
#include <multiboot/multiboot.h>

#include <system/serial.h>

void validate_mappings(uint32_t start_addr, uint32_t end_addr) {
	int valid = 1;
	for (uint32_t i = start_addr; i < end_addr; i += PAGE_SIZE) {
		uint32_t physical;
		uint32_t translated_virtual;

		const page_t *page = mmu_get_page(i, mmu_get_current_directory());

		if (i < KERNEL_VIRTUAL_BASE) {
			// Identity mapping (if needed)
			physical = i;
			translated_virtual = physical;
		} else {
			// Higher half mapping
			physical = mmu_get_physical_address((void *)i);
			translated_virtual = mmu_get_virtual_address(physical);
		}
		if (translated_virtual != i) {
			qemu_printf("Mapping Validation Failed for VA=0x%x: PA=0x%x, Expected VA=0x%x\n", i, physical, translated_virtual);
			valid = 0;
		}

		if (page == NULL) {
			qemu_printf("Page not found at VA=0x%x\n", i);
			valid = 0;
		} else if (!(page->rw && page->present)) {
			qemu_printf("Page not present or not writable at VA=0x%x\n", i);
			valid = 0;
		}
	}
	if (valid) {
		printk("All mappings from 0x%x to 0x%x are valid.\n", start_addr, end_addr);
		qemu_printf("All mappings from 0x%x to 0x%x are valid.\n", start_addr, end_addr);
	} else {
		__PANIC("Some mappings are invalid");
	}
}

int mmu_compare_page_directories(page_directory_t *dir1, page_directory_t *dir2) {
	if (dir1 == dir2) return 1; // Same directory
	int identical = 1;			// Assume directories are identical initially

	for (int i = 0; i < PAGE_ENTRIES; ++i) {
		if (dir1->tables[i] && dir2->tables[i]) {
			for (int j = 0; j < PAGE_ENTRIES; ++j) {
				page_t *page1 = &dir1->tables[i]->pages[j];
				page_t *page2 = &dir2->tables[i]->pages[j];
				if (page1->present != page2->present ||
					page1->rw != page2->rw ||
					page1->user != page2->user ||
					page1->accessed != page2->accessed ||
					page1->dirty != page2->dirty ||
					page1->frame != page2->frame) {
					identical = 0;
				}
			}
		} else if (dir1->tables[i] || dir2->tables[i]) {
			identical = 0;
		}
	}
	return identical;
}

void verify_page_protection(uint32_t address, uint32_t size, int expected_permissions) {
	for (uint32_t i = address; i < address + size; i += PAGE_SIZE) {
		page_t *page = mmu_get_page(i, mmu_get_current_directory());
		if (page == NULL) {
			qemu_printf("Page not found at address 0x%x\n", i);
			__PANIC("Page protection verification failed");
		}
		if ((page->rw != ((expected_permissions & PAGE_RW) != 0)) ||
			(page->user != ((expected_permissions & PAGE_USER) != 0))) {
			qemu_printf("Protection mismatch at address 0x%x\n", i);
			__PANIC("Page protection verification failed");
		}
	}
	printk("Page protection verified for address range 0x%x - 0x%x\n", address, address + size);
}

void validate_page_directory(page_directory_t *dir) {
	for (int i = 0; i < PAGE_ENTRIES; ++i) {
		if (dir->tables[i]) {
			for (int j = 0; j < PAGE_ENTRIES; ++j) {
				page_t *page = &dir->tables[i]->pages[j];
				if (page->present && page->frame == 0) {
					qemu_printf("Invalid frame address in page table entry at table %d, entry %d\n", i, j);
					__PANIC("Page directory validation failed");
				}
			}
		}
	}
	printk("Page directory validated successfully\n");
}

void display_page_directory(page_directory_t *dir) {
	for (int i = 0; i < PAGE_ENTRIES; ++i) {
		if (dir->tables[i]) {
			qemu_printf("Page Table %d: 0x%x\n", i, dir->tablesPhysical[i]);
			for (int j = 0; j < PAGE_ENTRIES; ++j) {
				page_t *page = &dir->tables[i]->pages[j];
				if (page->present) {
					qemu_printf("  Page %d: Frame 0x%x, RW %d, User %d, Accessed %d, Dirty %d\n", j, page->frame, page->rw, page->user, page->accessed, page->dirty);

					uint32_t physical = (i * PAGE_SIZE) + (j * PAGE_SIZE);
					uint32_t virtual = mmu_get_virtual_address(physical);
					qemu_printf("    Virtual Address: 0x%x, Physical Address: 0x%x\n", virtual, physical);
				}
			}
		}
	}
}

void mmu_display_available_frames() {
	uint32_t used_frames = get_used_frames();
	uint32_t total_frames = get_frame_count();

	qemu_printf("Used frames: %u / %u\n", used_frames, total_frames);
}

/**
 * @brief Check if paging is enabled.
 *
 * This function checks whether paging is enabled or not.
 *
 * @return 1 if paging is enabled, 0 otherwise.
 */
int mmu_is_paging_enabled() {
	uint32_t cr0;
	__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	return (cr0 & 0x80000000) ? 1 : 0;
}

/**
 * @brief Flushes the Translation Lookaside Buffer (TLB).
 *
 * The Translation Lookaside Buffer (TLB) is a cache that stores recently used virtual-to-physical address translations.
 * Flushing the TLB clears all the entries in the cache, forcing the CPU to perform new address translations.
 *
 * @note This function does not take any parameters.
 *
 * @return void
 */
void mmu_flush_tlb() {
	__asm__ __volatile__("mov %cr3, %eax; mov %eax, %cr3");
}

/**
 * @brief Flushes the Translation Lookaside Buffer (TLB) entry for the given address.
 *
 * @param address The address for which the TLB entry needs to be flushed.
 */
void mmu_flush_tlb_entry(uint32_t address) {
	__asm__ __volatile__("invlpg (%0)" : : "r"(address) : "memory");
}

/**
 * @brief Checks if the CPU is in protected mode.
 *
 * This function reads the CR0 control register and checks the PE (Protection Enable) bit.
 * If the PE bit (bit 0) is set, the CPU is in protected mode.
 *
 * @return uint8_t Returns 1 if the CPU is in protected mode, 0 otherwise.
 */
uint8_t mmu_is_protected_mode(void) {
	uint32_t cr0;
	__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	return (cr0 & 0x1);
}
