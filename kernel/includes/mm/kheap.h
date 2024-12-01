/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kheap.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/17 14:11:56 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:26:35 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef KHEAP_H
#define KHEAP_H

#include <stdbool.h> // bool
#include <stddef.h>	 // size_t
#include <stdint.h>	 // uint32_t

#include <mm/mmu.h> // page_directory_t

/* Define Alignment Constants */
#define HEAP_START_OFFSET (0x400000)						 // Offset from KERNEL_VIRTUAL_BASE
#define HEAP_START (KERNEL_VIRTUAL_BASE + HEAP_START_OFFSET) // Starting address of the heap (example)
#define HEAP_INITIAL_SIZE 0x100000							 // Initial heap size: 1 MB
#define HEAP_MAX_SIZE (0x40000000 - HEAP_START_OFFSET)		 // Maximum heap size: 1 GB
#define HEAP_PAGE_COUNT (HEAP_INITIAL_SIZE / PAGE_SIZE)

#define ALIGNMENT 0x1000 // Desired alignment

/* Magic Number for Heap Block Integrity */
#define HEAP_BLOCK_MAGIC 0xDEADBEEF

/* Structure représentant un bloc de mémoire dans le heap */
typedef struct heap_block {
	size_t size;			 // Size of the block
	bool is_free;			 // Free flag
	struct heap_block *next; // Next block in the heap
	struct heap_block *prev; // Previous block in the heap
	uint32_t magic;			 // Magic number for integrity
} heap_block_t;

/* Structure représentant le heap */
typedef struct heap {
	heap_block_t *first;   // First block in the heap
	heap_block_t *last;	   // Last block in the heap
	size_t size;		   // Current size of the heap
	page_directory_t *dir; // Page directory
} heap_t;

void initialize_heap(page_directory_t *dir);
void list_heap_blocks(void);

void *kmalloc(size_t size);
void *kmalloc_a(size_t size);
void *kmalloc_p(size_t size, uint32_t *phys);
void *kmalloc_ap(size_t size, uint32_t *phys);

void kfree(void *p);
void *kcalloc(size_t num, size_t size);
void *krealloc(void *p, size_t size);

size_t ksize(void *p);

void *vmalloc(size_t size);
void vfree(void *p);
void *vcalloc(size_t num, size_t size);
void *vrealloc(void *p, size_t size);

size_t vsize(void *p);

#endif /* !KHEAP_H */