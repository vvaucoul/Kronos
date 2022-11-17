/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   paging.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/17 14:29:43 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/11/17 15:00:38 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PAGING_H
#define PAGING_H

#include <kernel.h>
#include <asm/asm.h>
#include <system/pit.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 1024

#define MEMORY_END_PAGE 0x1000000 // 16MB

typedef struct s_page
{
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t unused : 7;
    uint32_t frame : 20;
} page_t;

typedef struct s_page_table
{
    page_t pages[PAGE_TABLE_SIZE];
} page_table_t;

typedef struct s_page_directory
{
    page_table_t *tables[PAGE_TABLE_SIZE];
    uint32_t tablesPhysical[PAGE_TABLE_SIZE];
    uint32_t physicalAddr;
} page_directory_t;

extern page_directory_t *kernel_directory;

extern void init_paging(void);
extern page_t *get_page(uint32_t address, page_directory_t *dir);
extern void page_fault(struct regs *r);

#endif /* !PAGING_H */