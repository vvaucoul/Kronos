/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pt_pool.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 13:35:47 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:23:33 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <bits.h>
#include <mm/ealloc.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <mm/pt_pool.h>
#include <stddef.h>
#include <string.h>
#include <system/panic.h>
#include <system/serial.h>

// Structure to represent the page table pool
typedef struct pagetable_pool {
	uint8_t *base;		  ///< Base of the allocated memory
	size_t size;		  ///< Total size of the pool
	size_t max_size;	  ///< Maximum size the pool can grow to
	size_t offset;		  ///< Current offset for allocations
	size_t alloc_size;	  ///< Allocation size for page tables
	uint32_t alloc_count; ///< Active allocation counter
	uint8_t *bitmap;	  ///< Bitmap for allocation management (1 bit per allocation)
} pagetable_pool_t;

static pagetable_pool_t page_table_pool;

/**
 * @brief Initializes the page table pool with appropriate alignment.
 */
void pagetable_pool_init(void) {
	// Define the allocation size for page tables (aligned to PAGE_SIZE)
	size_t alloc_size = (sizeof(page_table_t) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	// Allocate memory for the initial page table pool with alignment
	page_table_pool.base = (uint8_t *)ealloc_aligned(PAGE_TABLE_INITIAL_SIZE, PAGE_SIZE);
	if (!page_table_pool.base) {
		__PANIC("Failed to initialize the page table pool: Allocation failed");
	}

	// Initialize the allocated memory
	if (memset_s(page_table_pool.base, PAGE_TABLE_INITIAL_SIZE, 0, PAGE_TABLE_INITIAL_SIZE) != 0) {
		__PANIC("Failed to initialize the page table pool: Memory initialization failed");
	}

	// Allocate the bitmap
	size_t bitmap_size = (PAGE_TABLE_INITIAL_SIZE / alloc_size + 7) / 8; // 1 bit per allocation, rounded to the next byte
	page_table_pool.bitmap = (uint8_t *)ealloc(bitmap_size);
	if (!page_table_pool.bitmap) {
		__PANIC("Failed to initialize the page table pool: Bitmap allocation failed");
	}
	memset_s(page_table_pool.bitmap, bitmap_size, 0, bitmap_size); // Initialize the bitmap

	// Configure the pool
	page_table_pool.size = PAGE_TABLE_INITIAL_SIZE;
	page_table_pool.max_size = PAGE_TABLE_POOL_MAX_SIZE;
	page_table_pool.offset = 0;
	page_table_pool.alloc_size = alloc_size;
	page_table_pool.alloc_count = 0;

	qemu_printf("Page table pool initialized\n");
	qemu_printf("Base: %p -> %p\n", page_table_pool.base, page_table_pool.base + page_table_pool.size);
	qemu_printf("Size: %ld\n", page_table_pool.size);
	qemu_printf("Pool can manage %ld page tables\n", page_table_pool.size / page_table_pool.alloc_size);

	printk("\t   - Pool allocator, size: %ld, addr: %p, end addr: %p\n", PAGE_TABLE_INITIAL_SIZE, page_table_pool.base, page_table_pool.base + page_table_pool.size);
}

/**
 * @brief Dynamically expands the page table pool.
 *
 * @return 0 on success, -1 if unable to expand.
 */
static int pagetable_pool_expand(void) {
	if (page_table_pool.size >= page_table_pool.max_size) {
		return -1; // Pool cannot expand further
	}

	size_t new_size = page_table_pool.size * 2;
	if (new_size > page_table_pool.max_size) {
		new_size = page_table_pool.max_size;
	}

	uint8_t *new_base = (uint8_t *)ealloc_aligned(new_size, PAGE_SIZE);
	if (!new_base) {
		return -1; // Expansion failed
	}

	memcpy(new_base, page_table_pool.base, page_table_pool.size);

	// Update bitmap to handle the new size
	size_t old_bitmap_size = (page_table_pool.size / page_table_pool.alloc_size + 7) / 8;
	size_t new_bitmap_size = (new_size / page_table_pool.alloc_size + 7) / 8;
	uint8_t *new_bitmap = (uint8_t *)ealloc(new_bitmap_size);
	if (!new_bitmap) {
		__PANIC("Failed to expand the page table pool: Bitmap allocation failed");
	}
	memset_s(new_bitmap, new_bitmap_size, 0, new_bitmap_size);
	memcpy(new_bitmap, page_table_pool.bitmap, old_bitmap_size);

	// Free old memory
	// We replace efree with manual management, no actual efree
	// Logic to manage memory by MIN address and MAX address

	page_table_pool.base = new_base;
	page_table_pool.size = new_size;
	page_table_pool.bitmap = new_bitmap;

	qemu_printf("Page table pool expanded to size: %ld\n", page_table_pool.size);
	return 0;
}

/**
 * @brief Allocates a page table from the pool.
 *
 * @return Pointer to the allocated page table, or NULL if the pool is exhausted.
 */
page_table_t *pagetable_pool_alloc(void) {
	if (page_table_pool.alloc_count >= page_table_pool.size / page_table_pool.alloc_size) {
		// Try to expand the pool if possible
		if (pagetable_pool_expand() != 0) {
			qemu_printf("Page table pool exhausted\n");
			return NULL;
		}
	}

	// Search for the first free bit in the bitmap
	size_t index;
	for (index = 0; index < page_table_pool.size / page_table_pool.alloc_size; index++) {
		if (!(page_table_pool.bitmap[index / 8] & (1 << (index % 8)))) {
			break; // Free bit found
		}
	}

	// Mark the bit as allocated
	page_table_pool.bitmap[index / 8] |= (1 << (index % 8));
	page_table_pool.alloc_count++;

	// Allocate the page table
	page_table_t *addr = (page_table_t *)(page_table_pool.base + (index * page_table_pool.alloc_size));

	// Initialize the allocated memory
	if (memset_s(addr, sizeof(page_table_t), 0, sizeof(page_table_t)) != 0) {
		__PANIC("Failed to allocate page table: Memory initialization failed");
	}

	return addr;
}

/**
 * @brief Frees an allocated page table from the pool.
 *
 * @param addr Pointer to the page table to free.
 * @return 0 on success, error otherwise.
 */
int pagetable_pool_free(page_table_t *addr) {
	if (!addr) {
		return -1; // Null address
	}

	// Calculate the index in the bitmap
	size_t index = ((uint8_t *)addr - page_table_pool.base) / page_table_pool.alloc_size;

	// Check the validity of the index
	if (index >= page_table_pool.size / page_table_pool.alloc_size) {
		return -2; // Invalid index
	}

	// Check that the bit is allocated
	if (!(page_table_pool.bitmap[index / 8] & (1 << (index % 8)))) {
		return -3; // Bit already free
	}

	// Mark the bit as free
	page_table_pool.bitmap[index / 8] &= ~(1 << (index % 8));
	page_table_pool.alloc_count--;

	return 0; // Success
}

/**
 * @brief Verifies the integrity of the page table pool.
 *
 * @return 0 if the pool is consistent, error otherwise.
 */
int pagetable_pool_verify(void) {
	// Check the consistency between alloc_count and bitmap
	size_t alloc_count_from_bitmap = 0;
	for (size_t i = 0; i < (page_table_pool.size / page_table_pool.alloc_size + 7) / 8; i++) {
		alloc_count_from_bitmap += count_set_bits(page_table_pool.bitmap[i]);
	}
	if (alloc_count_from_bitmap != page_table_pool.alloc_count) {
		return -1; // Inconsistency between alloc_count and bitmap
	}

	// Check the validity of the memory range
	if (page_table_pool.base == NULL || page_table_pool.size == 0) {
		return -2; // Invalid memory range
	}

	// Other checks (optional, depending on specific needs)
	//...

	return 0; // Pool consistent
}

/**
 * @brief Tests the allocation of the entire pool to check if it can be filled.
 *
 * @return 0 on success, error otherwise.
 */
int pagetable_pool_full_test(void) {
	size_t total_allocatable = page_table_pool.size / page_table_pool.alloc_size;
	page_table_t **allocated_tables = (page_table_t **)ealloc(sizeof(page_table_t *) * total_allocatable);
	if (!allocated_tables) {
		return -1; // Allocation for test array failed
	}

	// Try to allocate the entire pool
	for (size_t i = 0; i < total_allocatable; i++) {
		allocated_tables[i] = pagetable_pool_alloc();
		if (!allocated_tables[i]) {
			// Free already allocated tables if allocation fails
			for (size_t j = 0; j < i; j++) {
				pagetable_pool_free(allocated_tables[j]);
			}

			return -2; // Pool could not be filled completely
		} else {
			printk("Allocated page table %ld at 0x%x\n", i, allocated_tables[i]);
		}
	}

	// Free all allocated tables
	for (size_t i = 0; i < total_allocatable; i++) {
		pagetable_pool_free(allocated_tables[i]);
	}

	return 0; // Success, pool filled completely
}
