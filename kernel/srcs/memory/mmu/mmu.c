/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmu.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/17 14:34:06 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/11/15 19:57:11 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <mm/mmu.h>	 // Memory Management Unit
#include <mm/mmuf.h> // Memory Management Unit Frames
#include <mm/pt_pool.h>

#include <mm/ealloc.h> // Early Alloc (ealloc, ealloc_aligned)

#include <mm/mm.h>				 // Memory Management
#include <multiboot/multiboot.h> // Multiboot (getmem)

#include <system/random.h>

#include <system/serial.h>
#include <time.h>

#include <workflows/workflows.h>

extern uint32_t kernel_stack;

extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

/**
 * @brief Retrieves the physical address corresponding to the given virtual address.
 *
 * @param virt_addr The virtual address.
 * @return The physical address.
 */
uint32_t mmu_get_physical_address(void *virt_addr) {
	uintptr_t va = (uintptr_t)virt_addr;
	if (va < KERNEL_VIRTUAL_BASE) {
		// Identity mapping for lower addresses
		return va;
	}
	// Higher-half mapping: VA = PA + KERNEL_VIRTUAL_BASE
	return va - KERNEL_VIRTUAL_BASE;
}

/**
 * @brief Retrieves the virtual address corresponding to the given physical address.
 *
 * @param physical_address The physical address for which to retrieve the virtual address.
 * @return The virtual address corresponding to the given physical address.
 */
uint32_t mmu_get_virtual_address(uint32_t physical_address) {
	return physical_address + KERNEL_VIRTUAL_BASE;
}

/**
 * Sets the NX (No-Execute) bit for the specified memory address.
 *
 * @param address The memory address to set the NX bit for.
 * @param enable  A flag indicating whether to enable or disable the NX bit.
 */
void mmu_set_nx_bit(uint32_t address, int enable) {
	page_t *page = mmu_get_page(address, current_directory);
	if (page) {
		page->nx = enable ? 1 : 0;
		mmu_flush_tlb_entry(address);
	}
}

/**
 * @brief Protects a memory region with specified permissions.
 *
 * This function is used to protect a memory region starting from the given address
 * with the specified size and permissions. The permissions parameter determines
 * the access rights for the protected region.
 *
 * @param address The starting address of the memory region.
 * @param size The size of the memory region to be protected.
 * @param permissions The permissions to be set for the memory region.
 */
void mmu_protect_region(uint32_t address, uint32_t size, int permissions) {
	for (uint32_t i = address; i < address + size; i += PAGE_SIZE) {
		page_t *page = mmu_get_page(i, current_directory);
		if (page) {
			page->rw = (permissions & 0x2) ? 1 : 0;
			page->user = (permissions & 0x4) ? 1 : 0;
			mmu_flush_tlb_entry(i);
		}
	}
}

void setup_higher_half_mapping(page_directory_t *dir) {
	// Allocate a new page table for the higher-half mapping
	page_table_t *higher_half_pt = pagetable_pool_alloc();
	// page_table_t *higher_half_pt = kmalloc_ap(sizeof(page_table_t), 0);
	if (!higher_half_pt) {
		qemu_printf("Failed to allocate higher-half page table.\n");
		__PANIC("Paging setup failed");
	}

	// Zero out the new page table
	memset(higher_half_pt, 0, sizeof(page_table_t));

	// Map physical addresses 0x0 - 0x3FFFFF to virtual addresses 0xC0000000 - 0xC03FFFFF
	for (uint32_t i = 0; i < 1024; i++) {
		page_t *page = &higher_half_pt->pages[i];
		allocate_frame(page, 1, PAGE_PRESENT | PAGE_RW); // is_kernel=1, is_writeable=1
	}

	// Assign the page table to the page directory
	dir->tables[KERNEL_PAGE_DIR_INDEX] = higher_half_pt;

	// Calculate the physical address of the page table
	uint32_t higher_half_pt_phys = mmu_get_physical_address((void *)higher_half_pt);

	// Set the physical address in tablesPhysical with present and write flags
	dir->tablesPhysical[KERNEL_PAGE_DIR_INDEX] = higher_half_pt_phys | PAGE_PRESENT | PAGE_RW;

	qemu_printf("Higher-half page table allocated at VA=%p, PA=0x%x\n", (void *)higher_half_pt, higher_half_pt_phys);
}

void test_page_allocation(page_directory_t *dir) {
	uint32_t test_address = 0xC0400000;						   // Test virtual address
	page_t *test_page = mmu_create_page(test_address, dir, 1); // Create a page
	if (!test_page || !test_page->present) {
		qemu_printf("test_page_allocation: Failed to create page at address %p\n", (void *)test_address);
		// __PANIC("test_page_allocation: Failed to create test page");
		return;
	}

	// Allocate a physical frame
	allocate_frame(test_page, 1, PAGE_PRESENT | PAGE_RW); // is_kernel=1, is_writeable=1
	if (test_page->frame == 0) {
		qemu_printf("test_page_allocation: Failed to allocate frame for test page\n");
		// __PANIC("test_page_allocation: Failed to allocate frame for test page");
		return;
	}
	qemu_printf("test_page_allocation: Test page successfully allocated at %p\n", (void *)test_address);
}

/**
 * @brief Initializes the memory management unit (MMU).
 *
 * This function initializes the MMU and configures the mappings for the higher half kernel.
 *
 * @return 0 on success, a negative error code on failure.
 */
int mmu_init(void) {
	if (mmu_is_protected_mode() == 0) {
		__PANIC("mmu_init: Not in protected mode");
	} else if (mmu_is_paging_enabled() == 0) {
		__PANIC("mmu_init: Paging is not enabled, must be enabled by Bootloader");
	}

	/* Register the page fault handler */
	isr_register_interrupt_handler(14, mmu_page_fault_handler);

	qemu_printf("End ADDR: 0x%x\n", get_placement_addr());

	/* Init frames */
	uint32_t mem_size = multiboot_get_available_memory();
	init_frames(mem_size);

	// Allocate memory for the new page directory with kmalloc_ap
	uint32_t new_dir_phys;
	page_directory_t *dir = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), &new_dir_phys);
	if (dir == NULL || new_dir_phys == 0) {
		qemu_printf("mmu_clone_page_directory: Failed to allocate memory for the new directory\n");
		return 1;
	}
	memset(dir, 0, sizeof(page_directory_t));
	qemu_printf("New page directory allocated at VA=%p, PA=0x%x\n", dir, new_dir_phys);

	// Correctly assign the physical address of the page directory
	// dir->physicalAddr = mmu_get_physical_address(dir->tablesPhysical);
	dir->physicalAddr = new_dir_phys;
	qemu_printf("Dir: 0x%x\n", dir);
	qemu_printf("Dir physicalAddr: 0x%x\n", dir->physicalAddr);

	// Assign the new directory to kernel_directory
	kernel_directory = dir;

	/* Initialize the page table pool before creating heap and mapping pages */
	pagetable_pool_init();

	/* Setup higher-half mapping */
	setup_higher_half_mapping(kernel_directory);

	/*
	 * Create and allocate page tables for the heap's virtual address space (HEAP_START to HEAP_START + HEAP_INITIAL_SIZE).
	 */
	for (uint32_t i = HEAP_START; i < (HEAP_START + HEAP_INITIAL_SIZE); i += PAGE_SIZE) {
		page_t *page = mmu_get_page(i, kernel_directory);

		if (page == NULL) {
			page = mmu_create_page(i, kernel_directory, 1);
		}
		allocate_frame(page, 1, PAGE_PRESENT | PAGE_RW);
	}
	qemu_printf("Create page from 0x%x to 0x%x\n", HEAP_START, HEAP_START + HEAP_INITIAL_SIZE);
	printk("Validate mappings [2]\n");
	qemu_printf("Validate mappings [2]\n");
	validate_mappings(HEAP_START, HEAP_START + HEAP_INITIAL_SIZE);

	/*
	 * Allocate frames for the kernel's physical memory
	 * Map first 4MB of physical memory to the kernel's virtual address space.
	 */
	for (uint32_t i = 0; i < get_placement_addr(); i += PAGE_SIZE) {
		page_t *page = mmu_get_page(i, kernel_directory);

		if (page == NULL) {
			page = mmu_create_page(i, kernel_directory, 1);
		}
		allocate_frame(page, 1, PAGE_PRESENT | PAGE_RW);
	}
	qemu_printf("Allocate frame from 0x%x to 0x%x\n", 0, get_placement_addr());
	printk("Validate mappings [3]\n");
	qemu_printf("Validate mappings [3]\n");
	validate_mappings(0x0, get_placement_addr()); // Corrected validation

	qemu_printf("Trying to switch page directory\n");
	/* Switch to the kernel page directory */
	mmu_switch_page_directory(kernel_directory);
	kpause();
	qemu_printf("Switch to kernel page directory\n");

	// test_page_allocation(kernel_directory);

	/* Initialize the heap */
	if (mmu_is_paging_enabled() == false) return (-1);
	initialize_heap(mmu_get_kernel_directory());

	printk("Heap created\n");
	qemu_printf("Heap created\n");

	// display_page_directory(kernel_directory);

	// workflow_heap();

	// uint32_t allocated = 0, i = 0;
	// while (1) {
	// 	const uint32_t alloc_size = 0x100000;
	// 	void *addr = kmalloc(alloc_size);

	// 	if (addr == NULL) {
	// 		qemu_printf("Failed to allocate memory\n");
	// 		break;
	// 	} else {
	// 		allocated += alloc_size;
	// 		printk("[%ld] Allocated: %ld Mo - Address: 0x%x\n", i, allocated / 1024 / 1024, addr);
	// 		qemu_printf("[%ld] Allocated: %ld Mo - Address: 0x%x\n", i, allocated / 1024 / 1024, addr);
	// 	}
	// }

	// // kpause();

	// kpause();
	// return (0);

	/*
	 * Now, for multi-tasking, we need to allocate a new page directory for the kernel.
	 * We cannot use the current page directory, as it is mapped in the kernel's virtual address space.
	 */

	// page_directory_t *new_kernel_directory = mmu_clone_page_directory(kernel_directory);
	// page_directory_t *new_kernel_directory = mmu_create_page_directory();

	// if (new_kernel_directory == NULL) {

	// 	printk("Failed to clone kernel page directory\n");
	// 	qemu_printf("Failed to clone kernel page directory\n");
	// 	return (-1);
	// }

	// /*
	//  * Switch to the new kernel page directory.
	//  * This will map the kernel's physical memory to the new page directory.
	//  */
	// mmu_switch_page_directory(new_kernel_directory);
	// qemu_printf("Switch to new kernel page directory\n");

	return (0);
}
