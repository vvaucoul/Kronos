/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   workflow_mmu.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/02 10:50:20 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/22 23:59:29 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <kernel.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <mm/mmuf.h>
#include <mm/shared.h>
#include <system/panic.h>
#include <workflows/workflows.h>

#include <system/pit.h>

#include <assert.h>
#include <string.h>

// Helper function to simulate page fault handler
void simulate_page_fault(uint32_t address) {
	struct regs r;
	// Fill r with relevant info, if needed
	r.eip = address;
	r.err_code = 0; // Adjust this as necessary
	mmu_page_fault_handler(&r);
}

// Test MMU initialization
void test_mmu_init() {
	assert(mmu_get_kernel_directory() != NULL);
	assert(mmu_is_paging_enabled() == 1);
	printk("test_mmu_init: "_GREEN
		   "[OK]\n"_END);
}

// Test getting and setting current directory
void test_mmu_directory_management() {
	page_directory_t *kernel_dir = mmu_get_kernel_directory();
	mmu_set_current_directory(kernel_dir);
	assert(mmu_get_current_directory() == kernel_dir);
	printk("test_mmu_directory_management: "_GREEN
		   "[OK]\n"_END);
}

// Test creating and retrieving a page
void test_mmu_page_creation() {
	page_directory_t *dir = mmu_get_kernel_directory();
	uint32_t test_address = 0x1000; // 4KB address
	page_t *page = mmu_create_page(test_address, dir, 1);
	assert(page != NULL);
	assert(page->present == 1);
	page_t *retrieved_page = mmu_get_page(test_address, dir);
	assert(retrieved_page == page);
	printk("test_mmu_page_creation: "_GREEN
		   "[OK]\n"_END);
}

// Test destroying a page
void test_mmu_page_destruction() {
	page_directory_t *dir = mmu_get_kernel_directory();
	uint32_t test_address = 0x1000;
	mmu_destroy_page(test_address, dir);
	page_t *page = mmu_get_page(test_address, dir);
	assert(page == NULL || page->present == 0);
	printk("test_mmu_page_destruction: "_GREEN
		   "[OK]\n"_END);
}

// Test cloning a page directory
void test_mmu_clone_directory() {
	page_directory_t *dir = mmu_get_kernel_directory();
	page_directory_t *clone = mmu_clone_page_directory(dir);
	assert(clone != NULL);

	if (mmu_compare_page_directories(dir, clone)) {
		printk("test_mmu_clone_directory: " _GREEN "[OK]\n" _END);
	} else {
		printk("test_mmu_clone_directory: " _RED "[FAIL]\n" _END);
	}

	// Verify flags for each cloned page
	for (int i = 0; i < PAGE_ENTRIES; ++i) {
		if (dir->tables[i] && clone->tables[i]) {
			for (int j = 0; j < PAGE_ENTRIES; ++j) {
				page_t *page1 = &dir->tables[i]->pages[j];
				page_t *page2 = &clone->tables[i]->pages[j];
				assert(page1->present == page2->present);
				assert(page1->rw == page2->rw);
				assert(page1->user == page2->user);
				assert(page1->accessed == page2->accessed);
				assert(page1->dirty == page2->dirty);
				// assert(page1->nx == page2->nx);
				assert(page1->frame == page2->frame);
			}
		}
	}

	printk("test_mmu_clone_directory (flags verified for cloned pages): " _GREEN "[OK]\n" _END);

	mmu_destroy_page_directory(clone);
}

// Test address translation
void test_mmu_address_translation() {
	page_directory_t *dir = mmu_get_kernel_directory();
	uint32_t test_address = 0xC0000000; // 3GB address
	mmu_create_page(test_address, dir, 1);
	uint32_t physical_address = mmu_get_physical_address((void *)test_address);
	assert(physical_address != 0);
	uint32_t virtual_address = mmu_get_virtual_address(physical_address);
	assert(virtual_address == test_address);
	printk("test_mmu_address_translation: "_GREEN
		   "[OK]\n"_END);
}

// Test memory protection
void test_mmu_protection() {
	page_directory_t *dir = mmu_get_kernel_directory();
	uint32_t test_address = 0x3000; // 12KB address
	mmu_create_page(test_address, dir, 1);
	mmu_protect_region(test_address, PAGE_SIZE, 0x2); // RW protection
	page_t *page = mmu_get_page(test_address, dir);
	assert(page->rw == 1);
	mmu_protect_region(test_address, PAGE_SIZE, 0x4); // User protection
	assert(page->user == 1);
	printk("test_mmu_protection: "_GREEN
		   "[OK]\n"_END);
}

// Test TLB flushing
void test_mmu_tlb_flush() {
	page_directory_t *dir = mmu_get_kernel_directory();
	uint32_t test_address = 0x4000; // 16KB address
	mmu_create_page(test_address, dir, 1);
	mmu_flush_tlb_entry(test_address);
	mmu_flush_tlb();
	printk("test_mmu_tlb_flush: "_GREEN
		   "[OK]\n"_END);
}

// Test handling of multiple page directories
void test_mmu_multiple_directories() {
	page_directory_t *dir1 = mmu_get_kernel_directory();
	page_directory_t *dir2 = mmu_clone_page_directory(dir1);
	assert(dir2 != NULL);

	// Modify a page in dir2 and verify that dir1 remains unchanged
	uint32_t test_address = 0x2000; // Arbitrary address
	mmu_create_page(test_address, dir2, 1);
	page_t *page_in_dir2 = mmu_get_page(test_address, dir2);
	assert(page_in_dir2 != NULL);
	assert(page_in_dir2->present == 1);

	page_t *page_in_dir1 = mmu_get_page(test_address, dir1);
	if (page_in_dir1 != NULL) {
		assert(page_in_dir1->present == 0); // dir1 should not have this page present
	} else {
		assert(page_in_dir1 == NULL); // dir1 should not have this page at all
	}

	printk("test_mmu_multiple_directories: "_GREEN
		   "[OK]\n"_END);

	mmu_destroy_page_directory(dir2);
}

void workflow_mmu(void) {
	__WORKFLOW_HEADER();

	if (mmu_is_paging_enabled() == true) {
		printk("MMU is enabled: "_GREEN
			   "[OK]\n"_END);
	} else {
		return;
	}

	test_mmu_init();
	test_mmu_directory_management();
	test_mmu_page_creation();
	test_mmu_page_destruction();

	test_mmu_clone_directory();
	test_mmu_address_translation();
	test_mmu_protection();
	test_mmu_tlb_flush();
	// test_mmu_multiple_directories();
	// kpause();

	__WORKFLOW_FOOTER();
}