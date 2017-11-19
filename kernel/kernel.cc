#include <stddef.h>
#include <stdint.h>
#include "io/serial.h"
#include "io/ports.h"
#include <cpuid.h>
#include "vsprintf.h"
#include "trace/trace.h"
#include "trace/stacktrace.h"
#include "interrupt/idt.h"
#include "interrupt/isr.h"
#include "pic/pic.h"
#include "multiboot.h"
#include "paging/pmm.h"
#include "paging/paging.h"
#include "tty/tty.h"
#include "tty/backends/vga_text.h"

void serial_writestr(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		serial_write_byte(data[i]);
}
 
void serial_writestring(const char* data) {
	serial_writestr(data, strlen(data));
}

void play_sound(uint32_t nFrequence) {
	uint32_t Div;
	uint8_t tmp;

    //Set the PIT to the desired frequency
	Div = 1193180 / nFrequence;
	outb(0x43, 0xb6);
	outb(0x42, (uint8_t) (Div) );
	outb(0x42, (uint8_t) (Div >> 8));

    //And play the sound using the PC speaker
	tmp = inb(0x61);
	if (tmp != (tmp | 3)) {
		outb(0x61, tmp | 3);
	}
}

static void nosound() {
	uint8_t tmp = inb(0x61) & 0xFC;

	outb(0x61, tmp);
}

void beepOn() {
	outb(0x61, inb(0x61) | 0x03);
}

void beepOff(uint32_t time)  {
	outb(0x61, inb(0x61) & ~0x03);
}


int printf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	tty_putstr(buf);
	return ret;
}

int kprintf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	serial_writestring(buf);
	return ret;
}

bool axc = false;
char lastkey = '\0';

bool __attribute__((noreturn)) page_fault(interrupt_cpu_state *state) {
	uint32_t fault_addr;
   	asm volatile("mov %%cr2, %0" : "=r" (fault_addr));

	// The error code gives us details of what happened.
	int present   = !(state->err_code & 0x1); // Page not present
	int rw = state->err_code & 0x2;           // Write operation?
	int us = state->err_code & 0x4;           // Processor was in user-mode?
	int reserved = state->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = state->err_code & 0x10;          // Caused by an instruction fetch?

	// Output an error message.
	tty_putstr("Page fault (");
	if (present) {tty_putstr("present");}
	if (rw) {tty_putstr(",read-only");}
	if (us) {tty_putstr(",user-mode");}
	if (id) {tty_putstr(",instruction-fetch");}
	if (reserved) {tty_putstr(",reserved");}
	printf(") at 0x%08x\n", fault_addr); 
	asm volatile ("1:\nhlt\njmp 1b");
}

const char lower_normal[] = { '\0', '?', '1', '2', '3', '4', '5', '6',     
		'7', '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 
				'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g', 
				'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 
				'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '};

bool a(interrupt_cpu_state *state) {

	uint8_t c = inb(0x60);
	
	if (c < 0x57)
		printf("%c", lower_normal[c]);

	return true;

}

extern "C" { 

void setup_gdt(void);

void kernel_main(multiboot_info_t *d) {
	/* Initialize terminal interface */


	uint16_t div;
	div = 1193180 / 500;
	outb(0x43, 0xB6);
	outb(0x41, div & 0xFF);
	outb(0x42, div >> 8);
	outb(0x40, 0xA9);
	outb(0x40, 0x4);

	tty_setdev(vga_text_tty_dev);
	tty_init();


	if (((uint32_t)d) - 0xC0000000 > 0x800000) {
		tty_putstr("[boot] mboot hdr out of page");
		while(1);	
	}

	printf("[boot] mboot: 0x%x\n", ((uint32_t)d));

	setup_gdt();
	tty_putstr("GDT ok\n");

	pic_remap(0x20, 0x28);

	idt_init();
	asm volatile ("sti");
	tty_putstr("IDT ok\n");

	
	uint32_t brand[12];
	__cpuid(0x80000002 , brand[0], brand[1], brand[2], brand[3]);
	__cpuid(0x80000003 , brand[4], brand[5], brand[6], brand[7]);
	__cpuid(0x80000004 , brand[8], brand[9], brand[10], brand[11]);

	printf("Hello world! CPU brand: %s\n", (const char*)brand);	
	//kprintf("Hello world! CPU brand: %s\n", (const char*)brand);

	register_interrupt_handler(0x21, a);
	register_interrupt_handler(14, page_fault);

	printf("[boot] params: %s\n", (const char*)(0xC0000000 + d->cmdline));
	printf("[boot] fbuf: 0x%x\n", d->framebuffer_addr);

	pmm_init(d->mem_upper);

	paging_init();

	while(1);
	
	asm volatile ("int $0x00");

}

}