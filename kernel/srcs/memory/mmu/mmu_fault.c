/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmu_fault.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/11/17 14:59:44 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/23 18:43:27 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/******************************************************************************/
/*                                                                            */
/*                       PAGE FAULT HANDLER DOCUMENTATION                     */
/*                                                                            */
/* ========================================================================== */
/*                                                                            */
/*    The page fault handler is designed to handle exceptions caused by       */
/*    invalid memory accesses. The handler provides detailed information      */
/*    about the nature of the fault and the state of the system when the      */
/*    fault occurs.                                                           */
/*                                                                            */
/* ========================================================================== */
/*                                                                            */
/*    FUNCTION:                                                               */
/*        void page_fault(struct regs *r)                                     */
/*                                                                            */
/*    DESCRIPTION:                                                            */
/*        Handles page fault exceptions by printing detailed information      */
/*        about the fault and the state of the registers at the time of the   */
/*        fault. The function also checks the faulting address and provides   */
/*        specific panic messages based on the type of fault detected.        */
/*                                                                            */
/*    PARAMETERS:                                                             */
/*        struct regs *r - Pointer to the register structure containing the   */
/*                         state of the registers at the time of the fault.   */
/*                                                                            */
/*    FAULT DETAILS:                                                          */
/*        The page fault handler decodes the error code to determine the      */
/*        cause of the fault. The error code is examined to check for the     */
/*        following conditions:                                               */
/*            - Page not present                                              */
/*            - Read-only violation                                           */
/*            - User-mode access                                              */
/*            - Reserved bits violation                                       */
/*            - Instruction fetch violation                                   */
/*                                                                            */
/*    SAMPLE OUTPUT:                                                          */
/*        The handler prints the following information to the console:        */
/*            - Faulting address                                              */
/*            - Error code interpretation                                     */
/*            - CR3 register value                                            */
/*            - Register state                                                */
/*            - Specific panic message based on the type of fault             */
/*                                                                            */
/******************************************************************************/

#include <drivers/tty.h>
#include <mm/memory.h>
#include <mm/mm.h>
#include <mm/mmu.h>
#include <system/panic.h>

#include <system/serial.h>

/**
 * @brief Reads the value of the CR2 register.
 *
 * This function reads the value of the CR2 register and stores it in the provided pointer.
 *
 * @param cr2 Pointer to a uint32_t variable where the value of the CR2 register will be stored.
 */
static void read_cr2(uint32_t *cr2) {
	__asm__ volatile("movl %%cr2, %0" : "=r"(*cr2));
}

/**
 * @brief Reads the value of the CR3 register.
 *
 * This function reads the value of the CR3 register and stores it in the provided pointer.
 *
 * @param cr3 Pointer to a uint32_t variable where the value of the CR3 register will be stored.
 */
static void read_cr3(uint32_t *cr3) {
	__asm__ volatile("movl %%cr3, %0" : "=r"(*cr3));
}

/**
 * @brief Dump page information.
 *
 * This function is responsible for dumping page information given the registers and the address.
 *
 * @param r A pointer to the `regs` structure containing register information.
 * @param address The address of the page.
 */
static void dump_page_info(struct regs *r, uint32_t address) {
	page_t *page = mmu_get_page(address, mmu_get_current_directory());
	if (page) {
		printk("Page details for address 0x%08x (EIP: 0x%08x):\n", address, r->eip);
		printk("Present: %d, RW: %d, User: %d, Accessed: %d, Dirty: %d, Frame: 0x%08x\n",
			   page->present, page->rw, page->user, page->accessed, page->dirty, page->frame << 12);
	} else {
		printk("No page information available for address 0x%08x (EIP: 0x%08x)\n", address, r->eip);
	}
}

/**
 * @brief Handles the page fault exception in the MMU.
 *
 * This function is responsible for handling the page fault exception in the MMU.
 * It takes a pointer to a struct `regs` as a parameter, which contains the register
 * values at the time of the exception.
 *
 * @param r A pointer to a struct `regs` containing the register values.
 */
void mmu_page_fault_handler(struct regs *r) {
	uint32_t faulting_address, cr3;

	read_cr2(&faulting_address);

	int present = !(r->err_code & 0x1); // Page not present
	int rw = r->err_code & 0x2;			// Write operation
	int us = r->err_code & 0x4;			// Processor was in user-mode
	int reserved = r->err_code & 0x8;	// Overwritten CPU-reserved bits of page entry
	int id = r->err_code & 0x10;		// Caused by an instruction fetch

	printk(_RED "Page fault! "_END
				"( ");
	if (present)
		printk(_YELLOW "present " _END);
	if (rw)
		printk(_YELLOW "read-only " _END);
	if (us)
		printk(_YELLOW "user-mode " _END);
	if (reserved)
		printk(_YELLOW "reserved " _END);
	if (id)
		printk(_YELLOW "instruction fetch "_END);
	printk(") at ");
	printk(_RED "0x%08x"_END, faulting_address);
	printk(" - EIP: 0x%08x\n", r->eip);

	/* Dump page information */
	dump_page_info(r, faulting_address);

	read_cr3(&cr3);
	printk("CR3: 0x%08x\n", cr3);

	printk(_END "\nInterrupt Frame:\n");
	printk("------------------------------------------------\n");
	printk("General Purpose Registers:\n");
	printk("  EAX: 0x%08x  EBX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x\n", r->eax, r->ebx, r->ecx, r->edx);
	printk("  EDI: 0x%08x  ESI: 0x%08x  EBP: 0x%08x  ESP: 0x%08x\n", r->edi, r->esi, r->ebp, r->esp);
	printk("\n");
	printk("Segment Registers:\n");
	printk("  GS: 0x%08x  FS: 0x%08x  ES: 0x%08x  DS: 0x%08x\n", r->gs, r->fs, r->es, r->ds);
	printk("\n");
	printk("Control Registers:\n");
	printk("  EIP: 0x%08x  CS: 0x%08x  SS: 0x%08x  EFLAGS: 0x%08x\n", r->eip, r->cs, r->ss, r->eflags);
	printk("  User ESP: 0x%08x\n", r->useresp);
	printk("------------------------------------------------\n");
	printk(_END);

	// Debug with QEMU Printf
	qemu_printf("Page fault at 0x%x (EIP: 0x%x)\n", faulting_address, r->eip);
	qemu_printf("Error code: 0x%x\n", r->err_code);
	qemu_printf("CR3: 0x%x\n", cr3);
	qemu_printf("General Purpose Registers:\n");
	qemu_printf("  EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n", r->eax, r->ebx, r->ecx, r->edx);
	qemu_printf("  EDI: 0x%x  ESI: 0x%x  EBP: 0x%x  ESP: 0x%x\n", r->edi, r->esi, r->ebp, r->esp);
	qemu_printf("Segment Registers:\n");
	qemu_printf("  GS: 0x%x  FS: 0x%x  ES: 0x%x  DS: 0x%x\n", r->gs, r->fs, r->es, r->ds);
	qemu_printf("Control Registers:\n");
	qemu_printf("  EIP: 0x%x  CS: 0x%x  SS: 0x%x  EFLAGS: 0x%x\n", r->eip, r->cs, r->ss, r->eflags);
	qemu_printf("  User ESP: 0x%x\n", r->useresp);

	// Determine the cause of the fault
	if (faulting_address == 0x0) {
		qemu_printf("Page fault at NULL pointer\n");
		__PANIC("Page fault at NULL pointer");
	} else if (faulting_address >= KERNEL_SPACE_START) {
		qemu_printf("Page fault at kernel address\n");
		__PANIC("Page fault at kernel address");
	} else if (reserved) {
		qemu_printf("Page fault at reserved address\n");
		__PANIC("Page fault at reserved address");
	} else if (rw && !present) {
		qemu_printf("Page fault trying to write to non-present page\n");
		__PANIC("Page fault trying to write to non-present page");
	} else if (!us && present) {
		qemu_printf("Page fault trying to execute kernel code\n");
		__PANIC("Page fault trying to execute kernel code");
	} else if (rw && us && present) {
		qemu_printf("Page fault trying to write to read-only page\n");
		__PANIC("Page fault trying to write to read-only page");
	} else {
		qemu_printf("Page fault\n");
		__PANIC("Page fault");
	}
}
