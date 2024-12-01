/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pt_pool.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/22 13:35:47 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 10:34:03 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PAGETABLE_POOL_H
#define PAGETABLE_POOL_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_TABLE_POOL_MAX_SIZE 0x400000 // Taille maximale de la pool
#define PAGE_TABLE_INITIAL_SIZE 0x100000  // Taille initiale de la pool

/**
 * @brief Initializes the page table pool.
 */
extern void pagetable_pool_init(void);

/**
 * @brief Allocates a page table from the pool.
 * @return Pointer to the allocated page table, or NULL if the pool is exhausted.
 */
extern page_table_t *pagetable_pool_alloc(void);

/**
 * @brief Frees an allocated page table from the pool.
 * @param addr Pointer to the page table to free.
 * @return 0 on success, error otherwise.
 */
extern int pagetable_pool_free(page_table_t *addr);

/**
 * @brief Verifies the integrity of the page table pool.
 * @return 0 if the pool is consistent, error otherwise.
 */
extern int pagetable_pool_verify(void);

/**
 * @brief Tests the allocation of the entire pool to check if it can be filled.
 * @return 0 on success, error otherwise.
 */
extern int pagetable_pool_full_test(void);

#endif /* !PAGETABLE_POOL_H */