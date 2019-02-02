include config.mk

OBJS = boot/boot.o kernel/kernel.o kernel/io/serial.o kernel/io/ports.o kernel/cpu/gdt.o kernel/vsprintf.o \
	   kernel/trace/stacktrace.o kernel/interrupt/idt_load.o kernel/interrupt/isr.o kernel/interrupt/idt.o \
	   kernel/interrupt/interrupt.o kernel/pic/pic.o \
	   kernel/paging/pmm.o kernel/paging/paging.o \
	   kernel/tasking/tasking.o kernel/cpu/gdt_new.o \
	   kernel/tasking/tasking_enter.o kernel/lib/string.o \
	   kernel/lib/stdlib.o kernel/lib/ctype.o kernel/kheap/liballoc.o kernel/kheap/liballoc_funcs.o \
	   kernel/panic.o kernel/mesg.o

CXX = clang -target i386-none-elf
CC = clang -target i386-none-elf
ASM = nasm
CXXFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++14 -Ikernel -Ikernel/lib -fno-omit-frame-pointer $(FLAGS_CXX) -mno-sse -mno-avx
CFLAGS = -ffreestanding -O2 -nostdlib -fno-omit-frame-pointer $(FLAGS_C)

kernel.elf: $(OBJS)
	@$(CC) -T linker.ld -o kernel.elf -lgcc $(CFLAGS) $^
	@printf "LINK\t\t%s\n" $@

%.o: %.cc
	@$(CXX) -c $< -o $@ $(CXXFLAGS)
	@printf "CXX\t\t%s\n" $@

%.o: %.asm
	@$(ASM) -g -felf32 -F dwarf $< -o $@
	@printf "ASM\t\t%s\n" $@

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
	-@rm quack.iso

