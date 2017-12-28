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
#include "kheap/heap.h"

void serial_writestr(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		serial_write_byte(data[i]);
}
 
void serial_writestring(const char* data) {
	serial_writestr(data, strlen(data));
}


int printf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	tty_putstr(buf);
	serial_writestring(buf);
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

// extern void map_page(void*,void*,int);

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
	else {tty_putstr("not present");}
	if (rw) {tty_putstr(", write");}
	else {tty_putstr(", read");}
	if (us) {tty_putstr(", user-mode");}
	else {tty_putstr(", kernel-mode");}
	if (id) {tty_putstr(", instruction-fetch");}
	if (reserved) {tty_putstr(", reserved");}
	printf(") at 0x%08x\n", fault_addr);
	stack_trace(20, 0);
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

void fastmemcpy(uint32_t dst, uint32_t src, uint32_t size) {
	asm volatile ("mov %0, %%ecx\nmov %1, %%edi\nmov %2, %%esi\nrep movsb" : : "r"(size), "r"(dst), "r"(src) : "%ecx","%edi","%esi");
}

extern "C" { 

void setup_gdt(void);

void kernel_main(multiboot_info_t *d) {
	/* Initialize terminal interface */
	if (((uint32_t)d) - 0xC0000000 > 0x800000) {
		tty_putstr("[kernel] mboot hdr out of page");
		return;
	}


	tty_setdev(vga_text_tty_dev);
	tty_init();
	serial_init();


	multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)(d->mmap_addr + 0xC0000000);
	while((uint32_t)mmap < (d->mmap_addr + 0xC0000000 + d->mmap_length)) {
		
		printf("%08p mem %08x - %x", mmap, mmap->addr, mmap->size);
		printf(" | %u\n", mmap->type);

		mmap = (multiboot_memory_map_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size));
	}


	setup_gdt();
	tty_putstr("[kernel] GDT ok\n");

	pic_remap(0x20, 0x28);
	tty_putstr("[kernel] PIC ok\n");

	idt_init();
	asm volatile ("sti");
	tty_putstr("[kernel] IDT ok\n");

	
	uint32_t brand[12];
	__cpuid(0x80000002 , brand[0], brand[1], brand[2], brand[3]);
	__cpuid(0x80000003 , brand[4], brand[5], brand[6], brand[7]);
	__cpuid(0x80000004 , brand[8], brand[9], brand[10], brand[11]);

	printf("[kernel] cpu brand: %s\n", (const char*)brand);	
	//kprintf("Hello world! CPU brand: %s\n", (const char*)brand);

	register_interrupt_handler(0x21, a);
	register_interrupt_handler(14, page_fault);

	printf("[kernel] params: %s\n", (const char*)(0xC0000000 + d->cmdline));
	printf("[kernel] fbuf: 0x%x\n", d->framebuffer_addr);

	pmm_init(d->mem_upper);

	paging_init();

	heap_init();
	// void *page = (void *)pmm_alloc();
	// map_page(page, (void*)0x0, 0x3);
	// page = (void *)pmm_alloc();
	// map_page(page, (void*)0x1000, 0x3);
	
	uint8_t *mem = (uint8_t *)kmalloc(30);

	printf("kmalloc: allocated 30 bytes at %08p\n", mem);

	for (int y = 0; y < 30; y ++) {
		
		if (((y % 8 == 0 && 30 - y > 8) || y == 30 - 1) && y) {
			printf(" | ");
			for (int i = 0; i < 8; i++) {
				char c = mem[y - 8 + i];
				printf("%c", c >= ' ' && c <= '~' ? c : '.');
			}
		}

		if (y == 0 || y % 8 == 0) {
			


			if (y % 8 == 0 && y) printf("\n");
			printf("%08x: ", 0 + y);

		}

		printf("%02x  ", mem[y]);

	}

	kfree(mem);

	uint32_t x = 100;

	while(1) { 	
		uint8_t* ptr = (uint8_t *)kmalloc(x * 4096);
		printf("free pages: %u/%u(%u bytes free)\n", free_pages(), max_pages(), free_pages() * 4096);
	}
	// printf("\n> ");


	while(1);
	
	asm volatile ("int $0x00");

}

}