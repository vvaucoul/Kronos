/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kheap.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/14 00:33:56 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/09/30 13:19:40 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef KHEAP_H
#define KHEAP_H

#include <kernel.h>
#include <memory/pmm.h>
#include <memory/memory_map.h>

#define SIZEOF_KBRK() (sizeof(void *))

#define PHYSICAL_START KMAP.available.start_addr
#define PHYSICAL_END KMAP.available.end_addr
#define PHYSICAL_LENGTH KMAP.available.length

#define PHYSICAL_MEMORY_BLOCKS 0x14

#define PHYSICAL_EXPAND_HEAP_START_SIZE 0x14
#define PHYSICAL_EXPAND_HEAP_START_OFFSET 0x20
#define PHYSICAL_EXPAND_HEAP_SIZE 0x1000

#define PHYSICAL_EXPAND_HEAP_START(size) (pmm_alloc_blocks(size))
#define PHYSICAL_EXPAND_HEAP_END(current_end, size) (void *)(current_end + (pmm_get_next_available_block() * PMM_BLOCK_SIZE + size))

typedef void data_t;

enum e_heap_block_state
{
    HEAP_BLOCK_FREE = 0,
    HEAP_BLOCK_USED = 1
};

typedef struct s_heap_block
{
    struct
    {
        uint32_t size;
        enum e_heap_block_state state;
    } metadata;
    struct s_heap_block *next;
    data_t *data;
} __attribute((packed)) t_heap_block;

typedef struct s_heap
{
    data_t *start_addr;
    data_t *end_addr;
    uint32_t max_size;
    uint32_t used_size;
    uint32_t allocated_blocks;
    t_heap_block *root;
} __attribute((packed)) t_heap;

#define HeapBlock t_heap_block
#define Heap t_heap

extern Heap kheap;

#define KHEAP_GET_MAX_SIZE() (kheap.max_size)
#define KHEAP_GET_USED_SIZE() (kheap.used_size)
#define KHEAP_GET_START_ADDR() (kheap.start_addr)
#define KHEAP_GET_END_ADDR() (kheap.end_addr)

extern int kheap_init(data_t *start_addr, data_t *end_addr);
extern data_t *kbrk(uint32_t size);
extern data_t *kmalloc(uint32_t size);
extern data_t *krealloc(void *ptr, uint32_t size);
extern uint32_t ksize(data_t *ptr);
extern void kfree(void *ptr);
extern void *kcalloc(uint32_t count, uint32_t size);
extern data_t *kmmap(uint32_t size);

#endif /* !KHEAP_H */