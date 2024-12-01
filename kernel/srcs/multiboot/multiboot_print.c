/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot_print.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/29 11:51:23 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/26 20:57:46 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <multiboot/multiboot.h>
#include <multiboot/multiboot_mmap.h>

#include <stdio.h>

/**
 * @brief Prints the multiboot information.
 *
 * This function is responsible for printing the multiboot information.
 * It can be used to display important details about the multiboot data
 * structure.
 */
void print_multiboot_info(void) {
	multiboot_info_t *mb_info = get_multiboot_info();
	printk("Multiboot Information Structure:\n");

	if (mb_info->flags & MULTIBOOT_FLAG_MEM) {
		printk("  Mem Lower: %u KB\n", mb_info->mem_lower);
		printk("  Mem Upper: %u KB\n", mb_info->mem_upper);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_DEVICE) {
		printk("  Boot Device: 0x%x\n", mb_info->boot_device);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_CMDLINE) {
		printk("  Command Line: %s\n", (char *)mb_info->cmdline);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_MODS) {
		printk("  Mods Count: %u\n", mb_info->mods_count);
		printk("  Mods Addr: 0x%x\n", mb_info->mods_addr);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_MMAP) {
		print_memory_map();
	}

	if (mb_info->flags & MULTIBOOT_FLAG_MODS) {
		printk("  Drives Length: %u\n", mb_info->drives_length);
		printk("  Drives Addr: 0x%x\n", mb_info->drives_addr);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_CONFIG) {
		printk("  Config Table: 0x%x\n", mb_info->config_table);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_LOADER) {
		printk("  Boot Loader Name: %s\n", (char *)mb_info->boot_loader_name);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_APM) {
		printk("  APM Table: 0x%x\n", mb_info->apm_table);
	}

	if (mb_info->flags & MULTIBOOT_FLAG_VBE) {
		printk("  VBE Control Info: 0x%x\n", mb_info->vbe_control_info);
		printk("  VBE Mode Info: 0x%x\n", mb_info->vbe_mode_info);
		printk("  VBE Mode: 0x%x\n", mb_info->vbe_mode);
		printk("  VBE Interface Seg: 0x%x\n", mb_info->vbe_interface_seg);
		printk("  VBE Interface Off: 0x%x\n", mb_info->vbe_interface_off);
		printk("  VBE Interface Len: 0x%x\n", mb_info->vbe_interface_len);
	}
}

/**
 * @brief Prints the memory map.
 *
 * This function is responsible for printing the memory map.
 * It retrieves the memory map information and displays it on the console.
 */
void print_memory_map(void) {
	multiboot_info_t *mb_info = get_multiboot_info();
	if (!(mb_info->flags & MULTIBOOT_FLAG_MMAP)) {
		printk("Memory map not provided by bootloader\n");
		return;
	}

	printk("Memory Map Length: %u\n", mb_info->mmap_length);
	printk("Memory Map Addr: 0x%x\n", mb_info->mmap_addr);

	multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mb_info->mmap_addr;
	uint32_t mmap_end = mb_info->mmap_addr + mb_info->mmap_length;
	while ((uint32_t)mmap < mmap_end) {
		printk("Size: 0x%x, Addr: 0x%llx, Len: 0x%llx, Type: 0x%x\n",
			   mmap->size, mmap->addr, mmap->len, mmap->type);
		mmap = (multiboot_mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
	}
}