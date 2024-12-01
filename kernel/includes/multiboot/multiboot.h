/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/04 13:28:32 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/26 22:01:29 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// Magic number for Multiboot 1
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// Multiboot memory map entry types
#define MULTIBOOT_MEMORY_AVAILABLE 1		// Available memory
#define MULTIBOOT_MEMORY_RESERVED 2			// Reserved memory
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3 // ACPI reclaimable memory
#define MULTIBOOT_MEMORY_NVS 4				// NVS memory
#define MULTIBOOT_MEMORY_BADRAM 5			// Bad RAM

// Multiboot header flags
#define MULTIBOOT_FLAG_MEM 0x001
#define MULTIBOOT_FLAG_DEVICE 0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS 0x008
#define MULTIBOOT_FLAG_AOUT 0x010
#define MULTIBOOT_FLAG_ELF 0x020
#define MULTIBOOT_FLAG_MMAP 0x040
#define MULTIBOOT_FLAG_CONFIG 0x080
#define MULTIBOOT_FLAG_LOADER 0x100
#define MULTIBOOT_FLAG_APM 0x200
#define MULTIBOOT_FLAG_VBE 0x400

/**
 * Multiboot structure representation
 *
 * +--------------------+---------------------+--------------------------+
 * | Offset (bytes)     | Size (bytes)        | Field                    |
 * +--------------------+---------------------+--------------------------+
 * | 0                  | 4                   | flags                    |
 * | 4                  | 4                   | mem_lower                |
 * | 8                  | 4                   | mem_upper                |
 * | 12                 | 4                   | boot_device              |
 * | 16                 | 4                   | cmdline                  |
 * | 20                 | 4                   | mods_count               |
 * | 24                 | 4                   | mods_addr                |
 * | 28                 | 4                   | num (syms)               |
 * | 32                 | 4                   | size (syms)              |
 * | 36                 | 4                   | addr (syms)              |
 * | 40                 | 4                   | shndx (syms)             |
 * | 44                 | 4                   | mmap_length              |
 * | 48                 | 4                   | mmap_addr                |
 * | 52                 | 4                   | drives_length            |
 * | 56                 | 4                   | drives_addr              |
 * | 60                 | 4                   | config_table             |
 * | 64                 | 4                   | boot_loader_name         |
 * | 68                 | 4                   | apm_table                |
 * | 72                 | 4                   | vbe_control_info         |
 * | 76                 | 4                   | vbe_mode_info            |
 * | 80                 | 4                   | vbe_mode                 |
 * | 84                 | 4                   | vbe_interface_seg        |
 * | 88                 | 4                   | vbe_interface_off        |
 * | 92                 | 4                   | vbe_interface_len        |
 * +--------------------+---------------------+--------------------------+
 */

// Multiboot header structure
typedef struct multiboot_info {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
	uint32_t mmap_length;
	uint32_t mmap_addr;
	uint32_t drives_length;
	uint32_t drives_addr;
	uint32_t config_table;
	uint32_t boot_loader_name;
	uint32_t apm_table;
	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint32_t vbe_mode;
	uint32_t vbe_interface_seg;
	uint32_t vbe_interface_off;
	uint32_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

// Multiboot memory map structure
typedef struct multiboot_mmap_entry {
	uint32_t size;
	uint64_t addr;
	uint64_t len;
	uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

// Function prototypes
extern int multiboot_init(uint32_t magic_number, uint32_t addr, uint32_t *kernel_stack);
extern multiboot_info_t *get_multiboot_info(void);

extern const char *multiboot_get_boot_loader_name(void);
extern const char *multiboot_get_cmdline(void);
extern const char *multiboot_get_device_name(void);
extern uint32_t multiboot_get_mem_lower(void);
extern uint32_t multiboot_get_mem_upper(void);
extern uint32_t multiboot_get_available_memory(void);

extern void print_multiboot_info(void);
extern void print_memory_map(void);

#endif /* !MULTIBOOT_H */