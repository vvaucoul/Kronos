# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2022/06/14 18:51:28 by vvaucoul          #+#    #+#              #
#    Updated: 2022/06/26 14:10:54 by vvaucoul         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#*******************************************************************************
#*                                     VAR                                     *
#*******************************************************************************

NAME		=	kfs
ISO			=	$(NAME).iso
LIBKFS		=	lkfs
LIBKFS_A	=	libkfs/libkfs.a
CC			=	gcc
CFLAGS		=	-Wall -Wextra -Werror -Wfatal-errors \
				-fno-builtin -fno-exceptions -fno-stack-protector \
				-nostdlib -nodefaultlibs \
				-std=gnu99 -ffreestanding -O2
LDFLAGS		= 	-g3 -m32

ASM			=	nasm
ASMFLAGS	=	-f elf32
BOOT		=	boot/boot
KDSRCS		=	srcs/kernel
BIN			=	kernel.bin
LINKER		=	linker.ld

XORRISO		=	xorriso-1.4.6
MK_INCLUDE_DIR	=	mk-files

#*******************************************************************************
#*                                  INLCUDES                                   *
#*******************************************************************************

include $(MK_INCLUDE_DIR)/Colors.mk

#*******************************************************************************
#*                                    KBOOT                                    *
#*******************************************************************************

KBOOT_SRCS	=	$(BOOT).s
KBOOT_OBJS	=	$(KBOOT_SRCS:.s=.o)

%.o: %.s
	@printf "$(_LWHITE)    $(_DIM)- Compiling: $(_END)$(_DIM)--------$(_END)$(_LPURPLE) %s $(_END)$(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n" $< 
	@$(ASM) $(ASMFLAGS) $< -o ${<:.s=.o}

#*******************************************************************************
#*                                    KSRCS                                    *
#*******************************************************************************

KSRCS_ASM	=	$(shell find kernel/srcs -name '*.s')
KOBJS_ASM	=	$(KSRCS_ASM:.s=.o)
KSRCS		=	$(shell find kernel/srcs -name '*.c')
KOBJS		=	$(KSRCS:.c=.o)
%.o: %.c
	@printf "$(_LWHITE)    $(_DIM)- Compiling: $(_END)$(_DIM)--------$(_END)$(_LCYAN) %s $(_END)$(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n" $< 
	@$(CC) $(LDFLAGS) $(CFLAGS) -c $< -o ${<:.c=.o}

#*******************************************************************************
#*                                    RULES                                    *
#*******************************************************************************

all: $(NAME)

$(NAME): ascii $(XORRISO) $(LIBKFS) $(BOOT) $(KDSRCS) $(BIN) $(ISO) helper

$(LIBKFS):
	@make -s -C libkfs
	@printf "$(_LWHITE)- LIBKFS$(_END)$(_END)$(_DIM)-----------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

$(BOOT): $(KBOOT_OBJS)
	@printf "$(_LWHITE)- ASM BOOT $(_END)$(_DIM)--------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

$(KDSRCS): $(KOBJS) $(KOBJS_ASM)
	@printf "$(_LWHITE)- KERNEL SRCS $(_END)$(_DIM)-----------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

$(BIN):
	@mkdir -p bin
	@$(CC) $(LDFLAGS) -T $(LINKER) -o bin/$(BIN) $(CFLAGS) \
	$(KBOOT_OBJS) $(KOBJS) $(KOBJS_ASM) $(LIBKFS_A) \
	> /dev/null 2>&1
	@printf "$(_LWHITE)    $(_DIM)- Compiling: $(_END)$(_DIM)--------$(_END)$(_LYELLOW) %s $(_END)$(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n" "KERNEL / LINKER / BOOT" 
	@printf "$(_LWHITE)- KERNEL BIN $(_END)$(_DIM)------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)$(_DIM) -> ISO CREATION $(_END) \n"
	@make -s -C . check

$(XORRISO):
	@printf "$(_LYELLOW)- COMPILING XORRISO $(_END)$(_DIM)-----$(_END) $(_LYELLOW)[$(_LWHITE)⚠️ $(_LYELLOW)]$(_END) $(_LYELLOW)>$(_END)$(_DIM)$(shell pwd)/$(XORRISO)/$(_END)$(_LYELLOW)<$(_END)\n"
	@tar xf $(XORRISO).tar.gz
	@cd $(XORRISO) && ./configure --prefix=/usr > /dev/null 2>&1 && make -j$(nproc) > /dev/null 2>&1
	@printf "$(_LWHITE)- $(XORRISO) $(_END)$(_DIM)---------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

check:
	@grub-file --is-x86-multiboot bin/$(BIN) && printf "$(_LWHITE)- $(BIN) $(_END)$(_DIM)------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)" || printf "$(_LWHITE)- $(BIN) $(_END)$(_DIM)------------$(_END) $(_LRED)[$(_LWHITE)✗$(_LRED)]$(_END)"
	printf "$(_END)$(_DIM) -> ISO CHECKER $(_END)\n"

run:
	@printf "$(_LWHITE)Running $(_LYELLOW)KFS$(_LWHITE) with $(_LYELLOW)qemu-system-i386$(_LWHITE) with $(_LYELLOW)kernel$(_LWHITE) !\n"
	@qemu-system-i386 -smp 1 -kernel isodir/boot/$(BIN) -display sdl -vga std -full-screen 
 
run-iso:
	@printf "$(_LWHITE)Running $(_LYELLOW)KFS$(_LWHITE) with $(_LYELLOW)qemu-system-i386$(_LWHITE) with $(_LYELLOW)cdrom$(_LWHITE) !\n"
	@qemu-system-i386 -smp 1 -cdrom $(NAME).iso -display sdl -boot d -vga std -full-screen

run-curses:
	@printf "$(_LWHITE)Running $(_LYELLOW)KFS$(_LWHITE) with $(_LYELLOW)qemu-system-i386$(_LWHITE) with $(_LYELLOW)cdrom$(_LWHITE) !\n"
	@qemu-system-i386 -smp 1 -cdrom $(NAME).iso -display curses -vga std -full-screen

debug:
	@printf "$(_LWHITE)Running $(_LYELLOW)KFS$(_LWHITE) with $(_LYELLOW)qemu-system-i386$(_LWHITE) with $(_LYELLOW)kernel$(_LWHITE) in $(_LRED)debug mode$(_LWHITE) !\n"
	@qemu-system-i386 -smp 1 -kernel isodir/boot/$(BIN) -s -S -display sdl -vga std -full-screen

$(ISO):
	@mkdir -p isodir/boot/grub
	@cp bin/kernel.bin isodir/boot/kernel.bin
	@cp grub.cfg isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) isodir \
	--xorriso=$(shell pwd)/$(XORRISO)/xorriso/xorriso > /dev/null 2>&1
	@printf "$(_LWHITE)- ISO $(_END)$(_DIM)-------------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

clean:
	@make -s -C libkfs clean
	@rm -rf $(NAME).iso $(KBOOT_OBJS) isodir bin/$(BIN) $(KOBJS) $(KOBJS_ASM)
	@printf "$(_LWHITE)- CLEAN $(_END)$(_DIM)-----------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

fclean: clean
	@make -s -C libkfs fclean
	@rm -rf $(XORRISO) bin
	@printf "$(_LWHITE)- FCLEAN $(_END)$(_DIM)----------------$(_END) $(_LGREEN)[$(_LWHITE)✓$(_LGREEN)]$(_END)\n"

re: clean
	@make -s -C libkfs re
	@make -s -C . all

ascii:
	@printf "$(_LRED)\r██╗  ██╗███████╗███████╗$(_LWHITE)      $(_LRED) ██╗\n$(_END)"
	@printf "$(_LRED)\r██║ ██╔╝██╔════╝██╔════╝$(_LWHITE)      $(_LRED)███║\n$(_END)"
	@printf "$(_LRED)\r█████╔╝ █████╗  ███████╗$(_LWHITE)█████╗$(_LRED)╚██║\n$(_END)"
	@printf "$(_LRED)\r██╔═██╗ ██╔══╝  ╚════██║$(_LWHITE)╚════╝$(_LRED) ██║\n$(_END)"
	@printf "$(_LRED)\r██║  ██╗██║     ███████║$(_LWHITE)      $(_LRED) ██║\n$(_END)"
	@printf "$(_LRED)\r╚═╝  ╚═╝╚═╝     ╚══════╝$(_LWHITE)      $(_LRED) ╚═╝\n$(_END)"

helper:
	@printf "\n$(_LWHITE)- Now you use: \'$(_LYELLOW)make run$(_END)\' or \'$(_LYELLOW)make run-iso$(_END)\' to start the kernel !$(_END)\n"

.PHONY: all clean fclean re debug run run-iso ascii helper run-curses