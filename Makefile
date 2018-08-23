include config.mk

OBJS = boot/boot.o kernel/kernel.o kernel/io/serial.o kernel/io/ports.o kernel/cpu/gdt.o kernel/vsprintf.o \
	   kernel/trace/stacktrace.o kernel/interrupt/idt_load.o kernel/interrupt/isr.o kernel/interrupt/idt.o \
	   kernel/interrupt/interrupt.o kernel/pic/pic.o kernel/tty/tty.o \
	   kernel/paging/pmm.o kernel/paging/paging.o kernel/tty/backends/vesa_text.o \
	   kernel/tty/backends/vesa_font.o kernel/io/rtc.o kernel/kbd/ps2kbd.o \
	   kernel/tasking/tasking.o kernel/syscall/syscall.o kernel/cpu/gdt_new.o \
	   kernel/tasking/tasking_enter.o kernel/fs/vfs.o kernel/fs/devfs.o kernel/fs/ustar.o kernel/lib/string.o \
	   kernel/lib/stdlib.o kernel/lib/ctype.o kernel/kheap/liballoc.o kernel/kheap/liballoc_funcs.o \
	   kernel/tasking/elf.o kernel/devices/tty.o kernel/devices/initrd.o \
	   kernel/devices/mouse.o kernel/devices/videomode.o kernel/devices/uptime.o kernel/panic.o \
	   kernel/devices/time.o kernel/ubsan.o kernel/devices/serial.o kernel/mesg.o kernel/drivers/pci.o kernel/drivers/drv.o $(DRIVERS_OBJS)

CXX = i686-elf-g++
CC = i686-elf-gcc
ASM = i686-elf-as
ASM2 = nasm
CXXFLAGS = -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -std=c++14 -g -Ikernel -Ikernel/lib -fno-omit-frame-pointer $(FLAGS_CXX) $(DRIVERS_FLAGS)
CFLAGS = -ffreestanding -O0 -nostdlib -g -fno-omit-frame-pointer
iso: quack.iso

run: quack.iso
	@qemu-system-i386 -cdrom quack.iso -serial file:serial.txt -monitor stdio

quack.iso: kernel.elf
	@mkdir -p isodir/boot/grub
	@cp grub.cfg isodir/boot/grub/grub.cfg
	@cp kernel.elf isodir/boot/kernel.elf
	@make -C initrd all
	@cp initrd/initrd isodir/boot/initrd
	@grub-mkrescue -o quack.iso isodir > /dev/null 2>&1
	@echo "Done, thank you for waiting"

kernel.elf: kernel.o kernel/trace/trace.o
	@$(CC) -T linker.ld -o kernel.elf -lgcc $(CFLAGS) $^
	@printf "LINK\t\t%s\n" $@
	@rm kernel/trace/trace.o kernel/trace/trace.cc

kernel/trace/trace.o: kernel/trace/trace.cc
	@$(CXX) -c $< -o $@ $(CXXFLAGS) -Wno-narrowing
	@printf "CXX\t\t%s\n" $@

kernel/trace/trace.cc: kernel.o
	@python2 utils/trace_map.py > kernel/trace/trace.cc
	@printf "TRACE\t\tkernel.o\n"

kernel.o: $(OBJS)
	@$(CC) -T linker.ld -r -o kernel.o -lgcc $(CFLAGS) $^
	@printf "LINK\t\t%s\n" $@

%.o: %.cc
	@$(CXX) -c $< -o $@ -lgcc $(CXXFLAGS)
	@printf "CXX\t\t%s\n" $@

%.o: %.s
	$(ASM) $< -o $@

%.o: %.asm
	@$(ASM2) -g -felf32 -F dwarf $< -o $@
	@printf "ASM\t\t%s\n" $@

.PHONY: clean run

clean:
	@printf "cleaning\n"
	-@rm $(OBJS)
	-@rm kernel.elf
	-@rm kernel.o
	-@rm quack.iso
	
