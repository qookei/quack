OBJS = boot/boot2.o kernel/kernel.o kernel/io/serial.o kernel/io/ports.o kernel/cpu/gdt.o kernel/vsprintf.o \
	   kernel/trace/stacktrace.o kernel/interrupt/idt_load.o kernel/interrupt/isr.o kernel/interrupt/idt.o \
	   kernel/interrupt/interrupt.o kernel/pic/pic.o kernel/tty/tty.o kernel/tty/backends/vga_text.o \
	   kernel/paging/pmm.o kernel/paging/paging.o kernel/kheap/heap.o kernel/tty/backends/vesa_text.o \
	   kernel/tty/backends/vesa_font.o kernel/io/rtc.o kernel/kbd/ps2kbd.o

CXX = i686-elf-g++
CC = i686-elf-gcc
ASM = i686-elf-as
ASM2 = nasm
CXXFLAGS = -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -g -std=c++14
CFLAGS = -ffreestanding -O0 -nostdlib -g

run: quack.iso
	qemu-system-i386 -cdrom quack.iso -serial file:serial.txt -monitor stdio

debug-run: quack.iso
	echo "To debug, start gdb(or any debugger which uses gdb), load the kernel executable and execute the command \"target remote localhost:1234\""
	qemu-system-i386 -cdrom quack.iso -serial file:serial.txt -monitor stdio -s -S

quack.iso: kernel.elf
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp kernel.elf isodir/boot/kernel.elf
	grub-mkrescue -o quack.iso isodir

kernel.elf: kernel.o kernel/trace/trace.o
	$(CC) -T linker.ld -o kernel.elf -lgcc $(CFLAGS) $^
	rm kernel/trace/trace.o kernel/trace/trace.cc

kernel/trace/trace.o: kernel/trace/trace.cc
	$(CXX) -c $< -o $@ $(CXXFLAGS) -Wno-narrowing

kernel/trace/trace.cc:
	python utils/trace_map.py > kernel/trace/trace.cc

kernel.o: $(OBJS)
	$(CC) -T linker.ld -r -o kernel.o -lgcc $(CFLAGS) $^

%.o: %.cc
	$(CXX) -c $< -o $@ $(CXXFLAGS)

%.o: %.s
	$(ASM) $< -o $@

%.o: %.asm
	$(ASM2) -g -felf32 -F dwarf $< -o $@

.PHONY: clean run

clean:
	rm $(OBJS)
	rm kernel.elf
	rm kernel.o
	rm quack.iso