/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/10 19:02:46 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:19:13 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <multiboot/multiboot.h>
#include <multiboot/multiboot_mmap.h>

#include <asm/asm.h>
#include <mm/mm.h>

#include <kernel.h>
#include <macros.h>
#include <stddef.h>

#include <system/pit.h>

static multiboot_info_t *multiboot_info = NULL;
extern uint32_t initial_esp;
extern uint32_t *kernel_stack;

static void init_memory_map(void);

/**
 * @brief Checks if the magic number is valid for multiboot.
 *
 * This function checks if the provided magic number matches the expected
 * MULTIBOOT_BOOTLOADER_MAGIC value.
 *
 * @param magic_number The magic number to be checked.
 * @return 1 if the magic number is not valid, 0 otherwise.
 */
static int multiboot_check_magic_number(uint32_t magic_number) {
	if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
		return (1);
	}
	return (0);
}

/**
 * @brief Checks if the flag is set in the multiboot info structure.
 *
 * This function checks if the specified flag is set in the multiboot info
 * structure by performing a bitwise AND operation with the flags field.
 *
 * @param flag The flag to be checked.
 * @return 1 if the flag is set, 0 otherwise.
 */
static int multiboot_check_flag(uint32_t flag) {
	if (multiboot_info->flags & flag) {
		return (1);
	}
	return (0);
}

static int multiboot_init_kernel_stack(uint32_t *kstack) {
	/* Check if kernel stack is valid */
	if (kstack == NULL) {
		return (1);
	} else {
		kernel_stack = kstack;
		initial_esp = (uint32_t)(uintptr_t)kstack;
	}
	return (0);
}

/**
 * @brief Initializes the multiboot module.
 *
 * This function is responsible for initializing the multiboot module
 * with the given magic number and address.
 *
 * @param magic_number The magic number provided by the bootloader.
 * @param addr The address of the multiboot information structure.
 * @return void
 */
int multiboot_init(uint32_t magic_number, uint32_t addr, uint32_t *kernel_stack) {
	multiboot_info = (multiboot_info_t *)(uintptr_t)addr;

	/* Check if magic number is valid */
	if (multiboot_check_magic_number(magic_number) != 0) {
		__WARND("Invalid magic number", 1);
	}

	/**
	 *  Check if flags are valid
	 *
	 * Commented code is for flags that are currently not supported
	 *
	 */

	if (multiboot_check_flag(MULTIBOOT_FLAG_MEM) == 0) {
		__WARND("No memory information available", 1);
	}
	if (multiboot_check_flag(MULTIBOOT_FLAG_DEVICE) == 0) {
		__WARND("No device information available", 1);
	}
	if (multiboot_check_flag(MULTIBOOT_FLAG_CMDLINE) == 0) {
		__WARND("No command line available", 1);
	}
	if (multiboot_check_flag(MULTIBOOT_FLAG_MODS) == 0) {
		__WARND("No modules available", 1);
	}
	// if (multiboot_check_flag(MULTIBOOT_FLAG_AOUT) == 0) {
	// 	__WARND("No a.out symbol table available", 1);
	// }
	// if (multiboot_check_flag(MULTIBOOT_FLAG_ELF) == 0) {
	// 	__WARND("No ELF section header table available", 1);
	// }
	if (multiboot_check_flag(MULTIBOOT_FLAG_MMAP) == 0) {
		__WARND("No memory map available", 1);
	}
	// if (multiboot_check_flag(MULTIBOOT_FLAG_CONFIG) == 0) {
	// 	__WARND("No boot configuration table available", 1);
	// }
	// if (multiboot_check_flag(MULTIBOOT_FLAG_LOADER) == 0) {
	// 	__WARND("No boot loader name available", 1);
	// }
	if (multiboot_check_flag(MULTIBOOT_FLAG_APM) == 0) {
		__WARND("No APM table available", 1);
	}
	// if (multiboot_check_flag(MULTIBOOT_FLAG_VBE) == 0) {
	// 	__WARND("No VBE information available", 1);
	// }

	/* Initialize kernel stack */
	if ((multiboot_init_kernel_stack(kernel_stack)) != 0) {
		__WARND("Failed to initialize kernel stack", 1);
	}

	init_memory_map();

	return (0);
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                               INTERFACE FUNCTION                               ||
// ! ||--------------------------------------------------------------------------------||

/**
 * Retrieves the multiboot information structure.
 *
 * @return A pointer to the multiboot_info_t structure.
 */
multiboot_info_t *get_multiboot_info(void) {
	return (multiboot_info);
}

/**
 * Retrieves the name of the boot loader used to load the kernel.
 *
 * @return A pointer to a string containing the name of the boot loader.
 */
const char *multiboot_get_boot_loader_name(void) {
	return (const char *)(uintptr_t)multiboot_info->boot_loader_name;
}

/**
 * Retrieves the command line string from the multiboot structure.
 *
 * @return A pointer to the command line string.
 */
const char *multiboot_get_cmdline(void) {
	return (const char *)(uintptr_t)multiboot_info->cmdline;
}

/**
 * Retrieves the device name associated with the multiboot information.
 *
 * @return A pointer to a constant character string representing the device name.
 */
const char *multiboot_get_device_name(void) {
	uint32_t boot_device = multiboot_info->boot_device;

	switch (boot_device & 0xFF000000) {
		case 0x80000000:
			return "BIOS Boot Device";
		case 0x00000000:
			return "Hard Disk";
		case 0x88000000:
			return "CD-ROM";
		default:
			return "Unknown Device";
	}
}

/**
 * Retrieves the lower memory size.
 *
 * @return The lower memory size in bytes.
 */
uint32_t multiboot_get_mem_lower(void) {
	return (multiboot_info->mem_lower);
}

/**
 * @brief Retrieves the upper memory limit.
 *
 * This function returns the upper memory limit in bytes.
 *
 * @return The upper memory limit in bytes.
 */
uint32_t multiboot_get_mem_upper(void) {
	return (multiboot_info->mem_upper);
}

/**
 * @brief Retrieves the total available memory from the multiboot information.
 *
 * This function calculates the total available memory by summing the lower and upper memory
 * values obtained from the multiboot information.
 *
 * @return The total available memory in bytes.
 */
uint32_t multiboot_get_available_memory(void) {
	return (multiboot_get_mem_lower() + multiboot_get_mem_upper()) * 1024;
}

/**
 * @brief Initializes the memory map by reading the multiboot information structure.
 *
 * This function retrieves the multiboot information structure and checks if the memory map
 * is provided by the bootloader. If the memory map is available, it iterates through the
 * memory map entries and prints the available memory regions using the printk function.
 *
 * @note This function assumes that the multiboot information structure is already populated
 *       and accessible via the get_multiboot_info function.
 *
 * @details The memory map entries are processed to identify available memory regions. Each
 *          available memory region is printed in the format "Available Memory: start - end".
 *          The memory addresses are adjusted by adding the KERNEL_VIRTUAL_BASE to the
 *          physical addresses provided by the bootloader.
 *
 * @return void
 */
static void init_memory_map(void) {
	multiboot_info_t *mb_info = get_multiboot_info();

	if (!(mb_info->flags & MULTIBOOT_FLAG_MMAP)) {
		printk("Memory map not provided by bootloader\n");
		return;
	}

	uintptr_t mmap_addr = (uintptr_t)mb_info->mmap_addr + KERNEL_VIRTUAL_BASE;
	uintptr_t mmap_end = mmap_addr + mb_info->mmap_length;
	multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mmap_addr;

	while ((uintptr_t)mmap < mmap_end) {

		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
			printk("Available Memory: 0x%llx - 0x%llx\n", mmap->addr, mmap->addr + mmap->len);
		}

		mmap = (multiboot_mmap_entry_t *)((uintptr_t)mmap + mmap->size + sizeof(mmap->size));
	}
}
