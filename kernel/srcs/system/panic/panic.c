/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   panic.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/07/09 12:27:28 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 11:06:32 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <asm/asm.h>
#include <system/panic.h>

bool __panic_handler = false;

#if defined(__i386__)
static void __attribute__((unused)) __dump_registers(void) {
	uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip;

	__asm__ volatile("movl %%eax, %0" : "=r"(eax));
	__asm__ volatile("movl %%ebx, %0" : "=r"(ebx));
	__asm__ volatile("movl %%ecx, %0" : "=r"(ecx));
	__asm__ volatile("movl %%edx, %0" : "=r"(edx));
	__asm__ volatile("movl %%esi, %0" : "=r"(esi));
	__asm__ volatile("movl %%edi, %0" : "=r"(edi));
	__asm__ volatile("movl %%ebp, %0" : "=r"(ebp));
	__asm__ volatile("movl %%esp, %0" : "=r"(esp));
	__asm__ volatile("call 1f\n\t"
					 "1: pop %0\n\t" : "=r"(eip));

	printk(_END "\nRegister Dump:\n");
	printk("------------------------------------------------\n");
	printk("General Purpose Registers:\n");
	printk(_CYAN "  EAX: " _GREEN "0x%08x" _END "  " _CYAN "EBX: " _GREEN "0x%08x" _END "  " _CYAN "ECX: " _GREEN "0x%08x" _END "  " _CYAN "EDX: " _GREEN "0x%08x" _END "\n", eax, ebx, ecx, edx);
	printk(_CYAN "  EDI: " _GREEN "0x%08x" _END "  " _CYAN "ESI: " _GREEN "0x%08x" _END "  " _CYAN "EBP: " _GREEN "0x%08x" _END "  " _CYAN "ESP: " _GREEN "0x%08x" _END "\n", edi, esi, ebp, esp);
	printk("\n");
	printk("Control Registers:\n");
	printk(_CYAN "  EIP: " _GREEN "0x%08x" _END "\n", eip);
	printk("------------------------------------------------\n");
	printk(_END);
}
#endif

static void __panic_kerrno(void) {
	printk("\n");
	__DISPLAY_HEADER__();
	printk(_YELLOW "\nKerrno - Subsystem " _CYAN "[0]\n" _END);
	printk(_RED "Error:" _END " " _CYAN "%d\n" _END, KerrnoCode());
	printk(_RED "Sector:" _END " " _CYAN "%x\n" _END, KerrnoCodeSector());
	printk(_RED "Message:" _END " " _CYAN "%s" _END "\n" _END, KerrnoMessage());

	printk("\n");
	printk(_RED "File:" _END " " _CYAN "%s" _END "\n" _END, KerrnoFile());
	printk(_RED "Function:" _END " " _CYAN "%s" _END "\n" _END, KerrnoFunction());

	printk("\nYou must restart the system.\n");
}

static void __panic_multiple_kerrno(const char *str[], size_t count) {
	size_t i = 0;
	__DISPLAY_HEADER__();
	printk(_YELLOW "\nKerrno - Subsystem " _CYAN "[0]\n" _END);
	printk(_RED "Error:" _END " " _CYAN "%d\n" _END, KerrnoCode());
	printk(_RED "Sector:" _END " " _CYAN "%x\n" _END, KerrnoCodeSector());
	printk(_RED "Message:" _END " " _CYAN "%s" _END "\n" _END, KerrnoMessage());

	printk("\n");
	printk(_RED "File:" _END " " _CYAN "%s" _END "\n" _END, KerrnoFile());
	printk(_RED "Function:" _END " " _CYAN "%s" _END "\n" _END, KerrnoFunction());
	while (str[i] && i < count) {
		printk(_YELLOW "%s" _END "\n", str[i]);
		i++;
	}
	printk("\nYou must restart the system.\n");
}

void kernel_panic(const char *str) {
	if (__USE_KERRNO_HELPER__)
		__panic_kerrno();

	__DISPLAY_HEADER__();
	printk(_YELLOW "%s\n" _END, str);
	__PANIC_LOOP_HANDLER__();
}

void kernel_panic_multistr(const char *str[], size_t count) {
	if (__USE_KERRNO_HELPER__)
		__panic_multiple_kerrno(str, count);

	size_t i = 0;
	__DISPLAY_HEADER__();
	while (str[i] && i < count) {
		printk(_YELLOW "%s" _END "\n", str[i]);
		i++;
	}
	__PANIC_LOOP_HANDLER__();
}

// Gestion des erreurs fatales comme des paniques
void kernel_fault(const char *str) {
	__DISPLAY_HEADER_FAULT__();
	printk(_YELLOW "%s\n" _END, str);
}

void kernel_trap(const char *str) {
	__DISPLAY_HEADER_TRAP__();
	printk(_YELLOW "%s\n" _END, str);
}

#if defined(__GNUC__)
void kernel_panic_interrupt(const char *str, uint32_t index, panic_t fault, uint32_t code)
#elif defined(__clang__)
void __attribute__((no_caller_saved_registers)) kernel_panic_interrupt(const char *str, uint32_t index, panic_t fault, uint32_t code)
#endif
{
	if (__USE_KERRNO_HELPER__)
		__panic_kerrno();

	switch (fault) {
		case ABORT:
			__DISPLAY_HEADER__();
			printk(_RED "%s " _END "[" _RED "%ld" _END "] [" _RED "%ld" _END "]\n" _END, str, index, code);
			break;
		case FAULT:
			__DISPLAY_HEADER_FAULT__();
			printk(_END "%s " _END "[" _RED "%ld" _END "] [" _RED "%ld" _END "]\n" _END, str, index, code);
			break;
		case TRAP:
			__DISPLAY_HEADER_TRAP__();
			printk(_CYAN "%s " _END "[" _RED "%ld" _END "] [" _RED "%ld" _END "]\n" _END, str, index, code);
			break;
		case INTERRUPT:
			__DISPLAY_HEADER_INTERRUPT__();
			printk(_YELLOW "%s " _END "[" _RED "%ld" _END "] [" _RED "%ld" _END "]\n" _END, str, index, code);
			break;
	}
	__PANIC_LOOP_HANDLER__();
}

void __panic_loop_handler() {
#if defined(__i386__)
	// Todo: make it proper
	// __dump_registers();
#endif
	ASM_CLI();
	while (1) {
		;
	}
}