/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   smp.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/30 15:33:38 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/09/30 18:02:15 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <kernel.h>
#include <memory/smp.h>
#include <asm/asm.h>

/*
** SMP: Supervisor memory protection
*/

void *user_memcpy(void *destination, const void *source, size_t size)
{
    // Disable SMAP protections.
    SMP_SET_AC();
    // Perform normal memcpy.
    void *ret = kmemcpy(destination, source, size);
    // Restore SMAP protections.
    SMP_CLEAR_AC();
    return ret;
}

void cpu_cr4_set_bit(uint32_t bit)
{
    asm volatile("mov %0,%%cr4"
                 : "+r"(bit)
                 :
                 : "memory");

    // uint32_t cr4 = 0;
    // asm volatile("mov %%cr4, %0"
    //              : "=r"(cr4));
    // cr4 |= (1 << bit);
    // asm volatile("mov %0, %%cr4"
    //              :
    //              : "r"(cr4));
    // kprintf("CR4: %x\n", cr4);
}

static void __smp_init(void)
{
    uint32_t eax = 0x07;
    uint32_t ebx = 0;
    uint32_t ecx = 0;
    uint32_t edx = 0;

    CPUID(&eax, &ebx, &ecx, &edx);
    kprintf("CPUID: %x | %x | %x | %x\n", eax, ebx, ecx, edx);
    if (ebx & CPUID_SMEP)
    {
        cpu_cr4_set_bit(CPU_CR4_SMEP_BIT);
        kernel_log_info("LOG", "SMEP Enabled");
    }
    if (ebx & CPUID_SMAP)
    {
        cpu_cr4_set_bit(CPU_CR4_SMAP_BIT);
        kernel_log_info("LOG", "SMAP Enabled");
    }
    // SMP_SET_AC();
}

void smp_init(void)
{
    __smp_init();
}