/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pmm_regions.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/10/25 17:28:49 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/10/27 11:30:07 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <kernel.h>
#include <memory/pmm.h>
#include <system/panic.h>

PMM_REGION __pmm_region;

static void __pmm_region_init_regions(const pmm_physical_addr_t regions_bitmap, const pmm_physical_addr_t total_memory)
{
    kprintf("PMM: Init Regions: 0x%x | %u\n", regions_bitmap, total_memory);

    __pmm_region.regions = (struct __s_region *)regions_bitmap;
    __pmm_region.count = 0;
    __pmm_region.max_regions = total_memory / sizeof(struct __s_region) / PMM_BLOCK_SIZE;
    __pmm_region.start_addr = regions_bitmap;

    for (uint32_t i = 0; i < __pmm_region.max_regions; ++i)
    {
        __pmm_region.regions[i].start_addr = PMM_DEFAULT_ADDR;
        __pmm_region.regions[i].end_addr = PMM_DEFAULT_ADDR;
        __pmm_region.regions[i].size = 0;
        __pmm_region.regions[i].index = i;
    }
    __pmm_region.end_addr = __pmm_region.start_addr + (uint32_t)&__pmm_region.regions[__pmm_region.max_regions];

    kprintf("PMM_REGION: 0x%x\n", __pmm_region.regions);
    kprintf("PMM_REGION: 0x%x\n", __pmm_region.start_addr);
    kprintf("PMM_REGION: 0x%x\n", __pmm_region.end_addr);
    kprintf("PMM_REGION - Count: %u\n", __pmm_region.count);
    kprintf("PMM_REGION - MaxRegions: %u\n", __pmm_region.max_regions);
}

static pmm_region_t *__pmm_region_create_region(const pmm_physical_addr_t start, const pmm_physical_addr_t end)
{
    if (__pmm_region.count >= __pmm_region.max_regions)
        __PANIC(PMM_REGIONS_ERROR_CREATION);
    else
    {
        __pmm_region.regions[__pmm_region.count].start_addr = start;
        __pmm_region.regions[__pmm_region.count].end_addr = end;
        __pmm_region.regions[__pmm_region.count].size = end - start;
        __pmm_region.regions[__pmm_region.count].index = __pmm_region.count;
        ++__pmm_region.count;
    }
    return (&__pmm_region.regions[__pmm_region.count - 1]);
}

/*******************************************************************************
 *                           PMM REGION - INTERFACE                            *
 ******************************************************************************/

/*
** PMM - REGIONS INTERFACE FUNCTIONS
*/

void pmm_region_init_regions(const pmm_physical_addr_t regions_bitmap, const pmm_physical_addr_t total_memory)
{
    __pmm_region_init_regions(regions_bitmap, total_memory);
}

pmm_region_t *pmm_region_create_region(const pmm_physical_addr_t start, const pmm_physical_addr_t end)
{
    return (__pmm_region_create_region(start, end));
}

/*
** PMM - REGIONS USEFULL INTERFACE FUNCTIONS
*/
pmm_physical_addr_t pmm_region_get_start_addr(void)
{
    return (__pmm_region.start_addr);
}

pmm_physical_addr_t pmm_region_get_end_addr(void)
{
    return (__pmm_region.end_addr);
}

uint32_t pmm_region_get_count(void)
{
    return (__pmm_region.count);
}

uint32_t pmm_region_get_max_regions(void)
{
    return (__pmm_region.max_regions);
}