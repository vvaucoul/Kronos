/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kheap_tree.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/18 00:27:00 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/11/19 13:00:26 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <memory/kheap.h>
#include <memory/paging.h>

/*
static heap_node_t *__kheap_tree_create_node(uint32_t addr, uint32_t size, enum kheap_block_size block_size)
{
    kprintf("Creating node at 0x%08x\n", addr);
    heap_node_t *node = (heap_node_t *)(addr);

    node->infos.size = size;
    node->infos.status = USED;
    node->infos.addr = addr;
    node->infos.block_size = block_size;

    node->tree.left = NULL;
    node->tree.right = NULL;
    node->tree.parent = NULL;

    node->data = (void *)((uint32_t)(addr + sizeof(heap_node_t)));

    placement_addr += sizeof(heap_node_t) + size;

    return (node);
}

static void __kheap_tree_insert_node(heap_node_t **node, heap_node_t *new_node)
{
    if (*node == NULL)
    {
        *node = new_node;
        return;
    }

    if (new_node->infos.size < (*node)->infos.size)
    {
        if ((*node)->tree.left != NULL)
            __kheap_tree_insert_node(&(*node)->tree.left, new_node);
        else
        {
            (*node)->tree.left = new_node;
            new_node->tree.parent = *node;
        }
    }
    else if (new_node->infos.size > (*node)->infos.size)
    {
        if ((*node)->tree.right != NULL)
            __kheap_tree_insert_node(&(*node)->tree.right, new_node);
        else
        {
            (*node)->tree.right = new_node;
            new_node->tree.parent = *node;
        }
    }
}

static heap_node_t *__kheap_tree_find_node_by_size(heap_node_t *node, uint32_t size)
{
    if (node == NULL || size == 0)
        return (NULL);
    else
    {
        if (node->infos.size < size || node->infos.status == USED)
            return (__kheap_tree_find_node_by_size(node->tree.right, size));
        else
        {
            if (node->tree.left != NULL && node->tree.left->infos.size >= size && node->tree.left->infos.status == FREE)
                return (__kheap_tree_find_node_by_size(node->tree.left, size));
            else
                return (node);
        }
    }
    return (NULL);
}

static heap_node_t *__kheap_tree_find_addr(void *addr)
{
    heap_node_t *node = kheap->root;

    while (node != NULL)
    {
        if (node->data == addr)
            return (node);
        else if (node->data > addr)
            node = node->tree.left;
        else
            node = node->tree.right;
    }
    return (NULL);
}

static int __kheap_tree_select_heap(uint32_t size)
{
    if (size <= KHEAP_SIZE_LOW)
        return (KHEAP_INDEX_LOW);
    else if (size <= KHEAP_SIZE_MEDIUM)
        return (KHEAP_INDEX_MEDIUM);
    else
        return (KHEAP_INDEX_HIGH);
}

static heap_t *__kheap_get_heap(uint32_t size)
{
    kprintf("Heap %u\n", __kheap_tree_select_heap(size));
    return (&kheap[__kheap_tree_select_heap(size)]);
}

static void __kheap_tree_display_tree(heap_node_t *node)
{
    if (node == NULL)
        return;

    // kprintf("              0x%08x\n                   ^\n", node->tree.parent);
    kprintf("0x%08x <- "_GREEN
            "0x%08x"_END
            " -> 0x%08x\n",
            node->tree.left, node, node->tree.right);
    kprintf("                   ^\n");
    __kheap_tree_display_tree(node->tree.left);
    __kheap_tree_display_tree(node->tree.right);
}

uint32_t kheap_tree_get_node_size(void *ptr)
{
    const heap_node_t *node = __kheap_tree_find_addr(ptr);

    if (node == NULL)
    {
        kprintf("Node not found\n");
        return (0);
    }
    else
        return (node->infos.size);
}

void kheap_tree_defragment(void)
{
}

void kheap_tree_delete_node(void *ptr)
{
    heap_node_t *node = __kheap_tree_find_addr(ptr);

    if (node == NULL)
    {
        kprintf("Node not found\n");
        return;
    }
    else
    {
        node->infos.status = FREE;
        node->data = NULL;
    }
}

void *kheap_tree_alloc_memory(uint32_t size)
{
    heap_t *heap = __kheap_get_heap(size);
    const uint8_t heap_block_size = __kheap_tree_select_heap(size);

    assert(heap == NULL);
    kprintf("Heap 0x%x [%s]\n", heap, heap_block_size == 0 ? "LOW" : heap_block_size == 1 ? "MEDIUM"
                                                                                          : "HIGH");

    heap_node_t *new_node = __kheap_tree_find_node_by_size(heap->root, size);
    kprintf("Node: %s -> 0x%x\n", new_node == NULL ? "NULL" : "Valid !", new_node);
    if (new_node == NULL)
    {
        if (IS_ALIGNED(placement_addr) == false)
            ALIGN(placement_addr);
        new_node = __kheap_tree_create_node(placement_addr, size, heap_block_size);
        __kheap_tree_insert_node(&heap->root, new_node);
        kprintf("Node 0x%08x\n", new_node);
        kprintf("Data 0x%08x\n", new_node->data);
        kprintf("Placement addr 0x%x\n", placement_addr);
        __kheap_tree_display_tree(heap->root);
        return (new_node->data);
    }
    else
    {
        kprintf(_RED "Error: Node already exist\n"_END); // tmp
        kpause();
        // __kheap_tree_insert_node(heap->root, node);
        // update node
        kprintf("Node 0x%x\n", new_node);
        return (new_node + sizeof(heap_node_t));
    }
}
*/

static void __expand(uint32_t new_size, heap_t *heap)
{
    assert(new_size > heap->end_address - heap->start_address);

    if (new_size & 0xFFFFF000)
    {
        new_size &= 0xFFFFF000;
        new_size += 0x1000;
    }

    assert(heap->start_address + new_size <= heap->max_address);

    uint32_t old_size = heap->end_address - heap->start_address;
    uint32_t i = old_size;
    while (i < new_size)
    {
        const page_t *page = get_page(heap->start_address + i, kernel_directory);
        if (!page)
            page = create_page(heap->start_address + i, kernel_directory);
        alloc_frame(page, heap->supervisor ? 1 : 0, heap->readonly ? 1 : 0);
        i += 0x1000;
    }
    heap->end_address = heap->start_address + new_size;
}

static uint32_t __contract(uint32_t new_size, heap_t *heap)
{
    assert(new_size < heap->end_address - heap->start_address);

    if (new_size & 0x1000)
    {
        new_size &= 0x1000;
        new_size += 0x1000;
    }

    if (new_size < HEAP_MIN_SIZE)
        new_size = HEAP_MIN_SIZE;

    uint32_t old_size = heap->end_address - heap->start_address;
    uint32_t i = old_size - 0x1000;
    while (new_size < i)
    {

        const page_t *page = get_page(heap->start_address + i, kernel_directory);
        if (!page)
            page = create_page(heap->start_address + i, kernel_directory);
        free_frame(page);
        i -= 0x1000;
    }
    heap->end_address = heap->start_address + new_size;
    return (new_size);
}

static int32_t __find_smallest_hole(uint32_t size, uint8_t align, heap_t *heap)
{
    // Find the smallest hole that will fit.
    int32_t iterator = 0;
    while (iterator < heap->index.size)
    {
        heap_header_t *header = (heap_header_t *)lookup_ordered_array(iterator, &heap->index);
        // If the user has requested the memory be page-aligned
        if (align > 0)
        {
            // Page-align the starting point of this header.
            uint32_t location = (uint32_t)header;
            int offset = 0;
            if ((location + sizeof(heap_header_t)) & 0xFFFFF000 != 0)
                offset = 0x1000 /* page size */ - (location + sizeof(heap_header_t)) % 0x1000;
            int hole_size = (int)header->size - offset;
            // Can we fit now?
            if (hole_size >= (int)size)
                break;
        }
        else if (header->size >= size)
            break;
        iterator++;
    }
    // Why did the loop exit?
    if (iterator == heap->index.size)
        return -1; // We got to the end and didn't find anything.
    else
        return iterator;
}

void *alloc(uint32_t size, uint8_t align, heap_t *heap)
{
    uint32_t new_size = size + sizeof(heap_header_t) + sizeof(heap_footer_t);
    int32_t iterator = __find_smallest_hole(new_size, align, heap);

    if (iterator == -1)
    {
        uint32_t old_length = heap->end_address - heap->start_address;
        uint32_t old_end_address = heap->end_address;

        __expand(old_length + new_size, heap);
        uint32_t new_length = heap->end_address - heap->start_address;

        iterator = 0;
        int32_t idx = -1;
        uint32_t value = 0x0;

        while (iterator < heap->index.size)
        {
            uint32_t tmp = (uint32_t)lookup_ordered_array(iterator, &heap->index);
            if (tmp > value)
            {
                value = tmp;
                idx = iterator;
            }
            iterator++;
        }

        if (idx == -1)
        {
            heap_header_t *header = (heap_header_t *)old_end_address;
            header->magic = KHEAP_MAGIC;
            header->size = new_length - old_length;
            header->status = FREE;
            heap_footer_t *footer = (heap_footer_t *)(old_end_address + header->size - sizeof(heap_footer_t));
            footer->magic = KHEAP_MAGIC;
            footer->header = header;
            insert_ordered_array((void *)header, &heap->index);
        }
        else
        {
            heap_header_t *header = lookup_ordered_array(idx, &heap->index);
            header->size += new_length - old_length;
            heap_footer_t *footer = (heap_footer_t *)((uint32_t)header + header->size - sizeof(heap_footer_t));
            footer->header = header;
            footer->magic = KHEAP_MAGIC;
        }
        return (alloc(size, align, heap));
    }

    heap_header_t *orig_hole_header = (heap_header_t *)lookup_ordered_array(iterator, &heap->index);
    uint32_t orig_hole_pos = (uint32_t)orig_hole_header;
    uint32_t orig_hole_size = orig_hole_header->size;

    if (orig_hole_size - new_size < sizeof(heap_header_t) + sizeof(heap_footer_t))
    {
        size += orig_hole_size - new_size;
        new_size = orig_hole_size;
    }

    if (align && orig_hole_pos & 0xFFFFF000)
    {
        uint32_t new_location = orig_hole_pos + 0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(heap_header_t);
        heap_header_t *hole_header = (heap_header_t *)orig_hole_pos;
        hole_header->size = 0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(heap_header_t);
        hole_header->magic = KHEAP_MAGIC;
        hole_header->status = FREE;
        heap_footer_t *hole_footer = (heap_footer_t *)(new_location - sizeof(heap_footer_t));
        hole_footer->magic = KHEAP_MAGIC;
        hole_footer->header = hole_header;
        orig_hole_pos = new_location;
        orig_hole_size = orig_hole_size - hole_header->size;
    }
    else
    {
        remove_ordered_array(iterator, &heap->index);
    }

    heap_header_t *block_header = (heap_header_t *)orig_hole_pos;
    block_header->magic = KHEAP_MAGIC;
    block_header->status = USED;
    block_header->size = new_size;

    heap_footer_t *block_footer = (heap_footer_t *)(orig_hole_pos + sizeof(heap_header_t) + size);
    block_footer->magic = KHEAP_MAGIC;
    block_footer->header = block_header;

    if (orig_hole_size - new_size > 0)
    {
        heap_header_t *hole_header = (heap_header_t *)(orig_hole_pos + sizeof(heap_header_t) + size + sizeof(heap_footer_t));
        hole_header->magic = KHEAP_MAGIC;
        hole_header->status = FREE;
        hole_header->size = orig_hole_size - new_size;
        heap_footer_t *hole_footer = (heap_footer_t *)((uint32_t)hole_header + orig_hole_size - new_size - sizeof(heap_footer_t));
        if ((uint32_t)hole_footer < heap->end_address)
        {
            hole_footer->magic = KHEAP_MAGIC;
            hole_footer->header = hole_header;
        }
        insert_ordered_array((void *)hole_header, &heap->index);
    }
    return (void *)((uint32_t)block_header + sizeof(heap_header_t));
}

void free(void *p, heap_t *heap)
{
    if (p == 0)
        return ;

    heap_header_t *header = (heap_header_t *)((uint32_t)p - sizeof(heap_header_t));
    heap_footer_t *footer = (heap_footer_t *)((uint32_t)header + header->size - sizeof(heap_footer_t));

    assert(header->magic == KHEAP_MAGIC);
    assert(footer->magic == KHEAP_MAGIC);

    header->status = FREE;
    bool do_add = 1;

    heap_footer_t *test_footer = (heap_footer_t *)((uint32_t)header - sizeof(heap_footer_t));
    if (test_footer->magic == KHEAP_MAGIC && test_footer->header->status == FREE)
    {
        uint32_t cache_size = header->size;
        header = test_footer->header;
        footer->header = header;
        header->size += cache_size;
        do_add = 0;
    }

    heap_header_t *test_header = (heap_header_t *)((uint32_t)footer + sizeof(heap_footer_t));
    if (test_header->magic == KHEAP_MAGIC && test_header->status == FREE)
    {
        header->size += test_header->size;
        test_footer = (heap_footer_t *)((uint32_t)test_header + test_header->size - sizeof(heap_footer_t));
        footer = test_footer;
        uint32_t iterator = 0;
        while ((iterator < heap->index.size) && (lookup_ordered_array(iterator, &heap->index) != (void *)test_header))
            iterator++;
        assert(iterator < heap->index.size);
        remove_ordered_array(iterator, &heap->index);
    }

    if ((uint32_t)footer + sizeof(heap_footer_t) == heap->end_address)
    {
        uint32_t old_length = heap->end_address - heap->start_address;
        uint32_t new_length = __contract((uint32_t)header - heap->start_address, heap);
        if (header->size - (old_length - new_length) > 0)
        {
            header->size -= old_length - new_length;
            footer = (heap_footer_t *)((uint32_t)header + header->size - sizeof(heap_footer_t));
            footer->magic = KHEAP_MAGIC;
            footer->header = header;
        }
        else
        {
            uint32_t iterator = 0;
            while ((iterator < heap->index.size) && (lookup_ordered_array(iterator, &heap->index) != (void *)test_header))
                iterator++;
            if (iterator < heap->index.size)
                remove_ordered_array(iterator, &heap->index);
        }
    }

    if (do_add == 1)
        insert_ordered_array((void *)header, &heap->index);
}