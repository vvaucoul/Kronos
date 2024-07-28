/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   workflows.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/28 13:38:18 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/07/28 10:32:49 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __WORKFLOWS_H
#define __WORKFLOWS_H

#include <stdint.h>

#define WORKFLOW_CHAR '='

#define __WORKFLOW_HEADER() workflow_header(__FUNCTION__)
#define __WORKFLOW_FOOTER() workflow_footer();

#define __WHUGE_TESTS__ 0

/* Workflow Utils */
extern void workflow_header(const char *function_name);
extern void workflow_footer(void);
extern void workflow_loading(uint32_t new_progress);

/* Kernel Memory Map */
extern void display_kernel_memory_map(void);

/* Multiboot Info */
extern void display_multiboot_infos(void);

/* Physical memory manager */
extern void pmm_display(void);
extern int pmm_test(void);
extern int pmm_defragment_test(void);

/* Kernel Heap */
extern void kheap_test(void);

/* Interrupts test */
extern void interrupts_test(void);

/* Process test */
extern void process_test(void);

/* Threads test */
extern void threads_test(void);

/* IDE test */
extern void workflow_ide(void);

/* Filesystem TinyFS test */
void workflow_tinyfs(void);

/* Floppy Disk Controller test */
void workflow_fdc(void);

// ! ||--------------------------------------------------------------------------------||
// ! ||                                      UTILS                                     ||
// ! ||--------------------------------------------------------------------------------||

extern void task_dummy(void);

#endif /* !__WORKFLOWS_H */