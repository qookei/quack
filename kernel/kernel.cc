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

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};
 
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
 
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}
 
// size_t strlen(const char* str) {
// 	size_t len = 0;
// 	while (str[len])
// 		len++;
// 	return len;
// }
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 50;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
 
void terminal_initialize(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x3F);
    
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}

	serial_init();

}
 
void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
 
void terminal_putchar(char c) {
	serial_write_byte(c);
	if (c == '\n') {
		terminal_row ++;
		terminal_column = 0;

		if (terminal_row == VGA_HEIGHT-1)
			terminal_row = 0;
		
		return;
	}


	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}

	
}
 
void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}
 
void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

int printf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	terminal_writestring(buf);
	return ret;
}

bool a(interrupt_cpu_state *state) {

	printf("kbd\n");
	return true;

}

extern "C" { 

void setup_gdt(void);

void kernel_main(void) {
	/* Initialize terminal interface */
	terminal_initialize();
 
	/* Newline support is left as an exercise. */
	setup_gdt();

	terminal_writestring("GDT ok\n");

	idt_init();
	asm volatile ("sti");
	terminal_writestring("IDT ok\n");

	pic_remap(0x20, 0x28);

	uint32_t brand[12];
	__cpuid(0x80000002 , brand[0], brand[1], brand[2], brand[3]);
	__cpuid(0x80000003 , brand[4], brand[5], brand[6], brand[7]);
	__cpuid(0x80000004 , brand[8], brand[9], brand[10], brand[11]);
	
	printf("Hello world! CPU brand: %s\n", (const char*)brand);

	register_interrupt_handler(0x21, a);

	
	while(1);
	
	asm volatile ("int $0x00");

}

}