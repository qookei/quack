#include "vga_text.h"
#include "../../io/ports.h"

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
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
 
void vga_text_init(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x3F);
    
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xC00B8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}
 
void vga_text_setcolor(uint8_t color) {
	terminal_color = color;
}
 
void vga_text_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}


void *memset(void *arr, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char*)arr)[i] = (unsigned char)val;
		
	return arr;
}

void *memcpy(void *dest, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++) 
		((unsigned char*)dest)[i] = ((unsigned char*)src)[i];
		
	return dest;
}

void *memmove(void *dest, const void *src, size_t len) {
	unsigned char cpy[len];
	memcpy(cpy, src, len);
	return memcpy(dest, cpy, len);
}


void vga_text_putchar(char c) {
	//serial_write_byte(c);

	if (c == '\n') {
		terminal_column = 0;

		if (++terminal_row == VGA_HEIGHT) {
			terminal_row = VGA_HEIGHT-1;
			memmove((void*)(0xC00B8000), (void*)(0xC00B8000 + VGA_WIDTH * 2), VGA_WIDTH * 2 * VGA_HEIGHT-2);
			memset((void*)(0xC00B8000 + VGA_WIDTH * 2 * VGA_HEIGHT-1), 0, 0xA0);
		}


		
		return;
	}


	vga_text_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT){
			memmove((void*)(0xC00B8000), (void*)(0xC00B8000 + VGA_WIDTH * 2), VGA_WIDTH * 2 * VGA_HEIGHT-2);
			memset((void*)(0xC00B8000 + VGA_WIDTH * 2 * VGA_HEIGHT-1), 0, 0xA0);
			terminal_row = VGA_HEIGHT-1;
		}
	}

	
}
 
void vga_text_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		vga_text_putchar(data[i]);
}

extern size_t strlen(const char*);

void vga_text_writestring(const char* data) {
	vga_text_write(data, strlen(data));
}


void vga_text_tty_dev(tty_op op, uint32_t param) {

	switch(op) {
		case tty_op::INIT: {
			vga_text_init();
			break;
		}

		case tty_op::WRITE_CHAR: {
			vga_text_putchar((char)param);
			break;
		}

		case tty_op::WRITE_STRING: {
			vga_text_writestring((const char*)param);
			break;
		}

	}

}
