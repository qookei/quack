include config.mk

ifndef OLEVEL
OLEVEL = 2
endif

OBJS = boot/boot.o kernel/kernel.o kernel/io/serial.o kernel/io/ports.o kernel/cpu/gdt.o kernel/vsprintf.o \
	   kernel/trace/stacktrace.o kernel/interrupt/idt_load.o kernel/interrupt/isr.o kernel/interrupt/idt.o \
	   kernel/interrupt/interrupt.o kernel/pic/pic.o \
	   kernel/paging/pmm.o kernel/paging/paging.o \
	   kernel/sched/task.o kernel/cpu/gdt_new.o \
	   kernel/sched/task_enter.o kernel/lib/string.o \
	   kernel/lib/stdlib.o kernel/lib/ctype.o kernel/kheap/liballoc.o kernel/kheap/liballoc_funcs.o \
	   kernel/panic.o kernel/mesg.o kernel/sched/elf.o kernel/fs/ustar.o

CC=i686-elf-gcc
ASM = nasm
CFLAGS = -g -ffreestanding -O$(OLEVEL) -Wall -Wextra -std=gnu11 -Ikernel -Ikernel/lib -fno-omit-frame-pointer -mno-sse -mno-avx -nostdlib $(FLAGS_C)

kernel.elf: $(OBJS)
	@printf "LINK\t\t%s\n" $@
	@$(CC) -T linker.ld -o kernel.elf $^ $(CFLAGS)

%.o: %.c
	@printf "CC\t\t%s\n" $@
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	@printf "ASM\t\t%s\n" $@
	@$(ASM) -g -felf32 -F dwarf $< -o $@

config.mk:
	cd utils; \
	./menuconfig.sh

config:
	cd utils; \
	./menuconfig.sh

.PHONY: clean run config

clean:
	@printf "cleaning\n"
	-@rm $(OBJS)
	-@rm kernel.elf

