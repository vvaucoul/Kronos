/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pmm.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/10 12:06:36 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/10/15 19:12:36 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <memory/pmm.h>
#include <system/serial.h>

PMM_INFO pmm_info;

/*******************************************************************************
 *                            PRIVATE PMM FUNCTIONS                            *
 ******************************************************************************/

static int __pmm_init_blocks(uint32_t max_size)
{
    uint32_t i;

    i = 0;
    while (i < max_size)
    {
        pmm_info.blocks[i] = PMM_INIT_BLOCK();
        ++i;
    }
    return (0);
}

static int __pmm_get_first_free_block(void)
{
    uint32_t i;

    i = 0;
    while (i < pmm_info.infos.max_blocks)
    {
        if (pmm_info.blocks[i].state == PMM_BLOCK_FREE)
            return (i);
        ++i;
    }
    return (PMM_ERR_NO_FREE_BLOCK);
}

static int __pmm_get_first_free_block_by_size(uint32_t size)
{
    if (size == 0)
        return (PMM_ERR_NO_FREE_BLOCK);
    else if (size == 1)
        return (__pmm_get_first_free_block());
    else
    {
        for (uint32_t i = 0; i < pmm_info.infos.max_blocks; ++i)
        {
            bool is_valid = true;
            if (pmm_info.blocks[i].state != PMM_BLOCK_FREE)
                continue;
            for (uint32_t j = i; (i <= size) && (i + j < pmm_info.infos.max_blocks); j++)
            {
                if (pmm_info.blocks[j].state != PMM_BLOCK_FREE)
                {
                    is_valid = false;
                    break;
                }
            }
            if (is_valid)
                return (i);
        }
    }
    return (PMM_ERR_NO_FREE_BLOCK);
}

static void __pmm_set_block(uint32_t block_id, uint32_t size, enum e_pmm_block_state state)
{
    pmm_info.blocks[block_id].size = size;
    pmm_info.blocks[block_id].state = state;
    pmm_info.blocks[block_id].addr = (block_id * PMM_BLOCK_SIZE) + pmm_info.infos.memory_map_end;
}

static void *__pmm_alloc_block(void)
{
    if (PMM_OUT_OF_MEMORY(pmm_info) == true)
        return (NULL);
    else
    {
        int frame = __pmm_get_first_free_block();

        if (frame == PMM_ERR_NO_FREE_BLOCK)
            return (NULL);
        else
        {
            __pmm_set_block(frame, 1, PMM_BLOCK_USED);
            ++pmm_info.infos.used_blocks;
            return ((void *)pmm_info.blocks[frame].addr);
        }
    }
    return (NULL);
}

static void *__pmm_alloc_blocks(uint32_t size)
{
    if (PMM_OUT_OF_MEMORY(pmm_info) == true)
        return (NULL);
    else
    {
        int frame = __pmm_get_first_free_block_by_size(size);

        if (frame == PMM_ERR_NO_FREE_BLOCK)
            return (NULL);
        else
        {
            for (uint32_t i = 0; i <= size; i++)
                __pmm_set_block(frame + i, 1, PMM_BLOCK_USED);
            pmm_info.infos.used_blocks += size;
            return ((void *)pmm_info.blocks[frame].addr);
        }
    }
    return (NULL);
}

static void __pmm_free_block(void *addr)
{
    uint32_t i;

    i = 0;
    while (i < pmm_info.infos.max_blocks)
    {
        if (pmm_info.blocks[i].addr == (pmm_physical_addr_t)addr)
        {
            __pmm_set_block(i, 0, PMM_BLOCK_FREE);
            --pmm_info.infos.used_blocks;
            return;
        }
        ++i;
    }
}

static void __pmm_free_blocks(void *addr, uint32_t size)
{
    uint32_t i;

    i = 0;
    while (i < pmm_info.infos.max_blocks)
    {
        if (pmm_info.blocks[i].addr == (pmm_physical_addr_t)addr)
        {
            for (uint32_t j = 0; (j <= size) && (i + j < pmm_info.infos.max_blocks); j++)
                __pmm_set_block(i + j, 0, PMM_BLOCK_FREE);
            pmm_info.infos.used_blocks -= size;
            return;
        }
        ++i;
    }
}

static void __pmm_swap_blocks(uint32_t block1, uint32_t block2)
{
    PMM_BLOCK tmp = pmm_info.blocks[block1];
    pmm_info.blocks[block1] = pmm_info.blocks[block2];
    pmm_info.blocks[block2] = tmp;
}

/*
** Sort blocks by address (Simple Bubble Sort) (TODO : Improve)
*/
static void __pmm_defragment(void)
{
    uint32_t i;

    i = 0;
    while (i < pmm_info.infos.max_blocks)
    {
        if (pmm_info.blocks[i].state == PMM_BLOCK_FREE && pmm_info.blocks[i + 1].state != PMM_BLOCK_FREE)
        {
            __pmm_swap_blocks(i, i + 1);
            i = 0;
            continue;
        }
        else
            ++i;
    }
}

/*******************************************************************************
 *                            GLOBAL PMM FUNCTIONS                             *
 ******************************************************************************/

/* Init PMM */
int pmm_init(pmm_physical_addr_t bitmap, uint32_t total_memory_size)
{
    /* INIT PMM INFO */
    pmm_info.infos.memory_size = total_memory_size;
    pmm_info.infos.max_blocks = total_memory_size / PMM_BLOCK_SIZE;
    pmm_info.infos.memory_map_start = bitmap;
    pmm_info.infos.memory_map_length = pmm_info.infos.memory_map_end - pmm_info.infos.memory_map_start;

    kprintf("PMM : Memory Size : %u Mo\n", pmm_info.infos.memory_size / 1024 / 1024);
    kprintf("PMM : Max Blocks : %u\n", pmm_info.infos.max_blocks);
    kprintf("PMM : Memory Map Start : 0x%x\n", pmm_info.infos.memory_map_start);
    kprintf("PMM : Memory Map Length : 0x%x\n", pmm_info.infos.memory_map_length);

    /* INIT PMM BLOCKS */
    pmm_info.blocks = (pmm_block_t *)bitmap;
    __pmm_init_blocks(pmm_info.infos.max_blocks * sizeof(pmm_physical_addr_t));

    pmm_info.infos.used_blocks = 0;
    pmm_info.infos.memory_map_end = (uint32_t)(&(pmm_info.blocks[pmm_info.infos.max_blocks]));

    return (0);
}

/* Alloc */
void *pmm_alloc_block(void)
{
    return (__pmm_alloc_block());
}

void pmm_free_block(void *addr)
{
    __pmm_free_block(addr);
}

void *pmm_alloc_blocks(uint32_t size)
{
    return (__pmm_alloc_blocks(size));
}

void pmm_free_blocks(void *addr, uint32_t size)
{
    __pmm_free_blocks(addr, size);
}

/* Utils */
int pmm_get_next_available_block(void)
{
    return (__pmm_get_first_free_block());
}

int pmm_get_next_available_blocks(uint32_t size)
{
    return (__pmm_get_first_free_block_by_size(size));
}

/* Defragment */
void pmm_defragment(void)
{
    __pmm_defragment();
}

/*******************************************************************************
 *                                  PMM UTILS                                  *
 ******************************************************************************/

uint32_t pmm_get_max_blocks(void)
{
    return (pmm_info.infos.max_blocks);
}

uint32_t pmm_get_memory_size(void)
{
    return (pmm_info.infos.memory_size);
}

uint32_t pmm_get_memory_map_start(void)
{
    return (pmm_info.infos.memory_map_start);
}

uint32_t pmm_get_memory_map_end(void)
{
    return (pmm_info.infos.memory_map_end);
}

uint32_t pmm_get_memory_map_length(void)
{
    return (pmm_info.infos.memory_map_length);
}

uint32_t pmm_get_block_size(void)
{
    return (PMM_BLOCK_SIZE);
}

/*******************************************************************************
 *                                  TEST PMM                                   *
 ******************************************************************************/

void pmm_display_blocks(uint32_t size)
{
    uint32_t i;

    i = 0;
    while (i < size)
    {
        kprintf("Block %d: " COLOR_YELLOW "%s" COLOR_END ", addr: 0x%x\n", i,
                pmm_info.blocks[i].state == PMM_BLOCK_FREE ? "FREE" : "USED",
                pmm_info.blocks[i].addr);
        ++i;
    }
}