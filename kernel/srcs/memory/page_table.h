/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   page_table.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/16 14:56:03 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/08/16 15:07:56 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef _PAGE_TABLE_H
#define _PAGE_TABLE_H

typedef struct s_page_table
{
    int p : 1;     // Present
    int rw : 1;    // Read/Write
    int us : 1;    // User/Supervisor
    int pwt : 1;   // Page-Level Write-Through
    int pcd : 1;   // Page-Level Cache Disable
    int a : 1;     // Accessed
    int d : 1;     // Dirty
    int pat : 1;   // Page-Attribute Table
    int ps : 1;    // Page size
    int g : 1;     // Global
    int avail : 3; // Available
    int pfa : 20;  // Page Frame Address
} t_page_table;

/*
P: indicate if the page or table is in physical memory
• R/W: indicate if the page or table is accessible in writting (equals 1)
• U/S: equals 1 to allow access to non-preferred tasks
• A: indicate if the page or table was accessed
• D: (only for pages table) indicate if the page was written
• PS: (only for pages directory) indicate the size of pages :
◦ 0 = 4kb
◦ 1 = 4mb
Notes: Physical addresses in the pages directory or pages table are written using 20
bits because these addresses are aligned on 4kb, so the last 12bits should be equal to 0.
• A pages directory or pages table used 1024*4 = 4096 bytes = 4k
• A pages table can address 1024 * 4k = 4 Mb
• A pages directory can address 1024 (1024 4k) = 4 Gb
*/

#define PageTable t_page_table

static void enable_paging(PageTable page);
extern void __enable_paging(PageTable page)
{
    enable_paging(page);
}

#endif /* _PAGE_TABLE_H */