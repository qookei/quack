#include "vga_text.h"
#include "../../io/ports.h"

#define is_digit(c)	((c) >= '0' && (c) <= '9')

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

uint8_t bg;
uint8_t fg;


void vga_text_init(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x3F);
    
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	fg = VGA_COLOR_LIGHT_GREY;
	bg = VGA_COLOR_BLACK;
	
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
	
	if (c == '\n') {
		terminal_column = 0;

		if (++terminal_row == VGA_HEIGHT) {
			terminal_row = VGA_HEIGHT-1;
			memmove((void*)(0xC00B8000), (void*)(0xC00B8000 + VGA_WIDTH * 2), VGA_WIDTH * 2 * VGA_HEIGHT-2);
			memset((void*)(0xC00B8000 + VGA_WIDTH * 2 * VGA_HEIGHT-1), 0, 0xA0);
		}

		return;
	}

	if (c == '\r') {
		terminal_column = 0;
		return;
	}

	if (c == '\b') {
		terminal_column--;

		vga_text_putentryat(0x0, terminal_color, terminal_column, terminal_row);

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

int k_atoi(char *p) {
    int k = 0;
    while (*p) {
        k = (k<<3)+(k<<1)+(*p)-'0';
        p++;
     }
     return k;
}

bool ansi_seq = false;

char csi_num[32] = {0};
uint32_t csi_nums[32] = {0};

uint8_t csi_nums_idx = 0;
uint8_t csi_num_idx = 31;

uint32_t cur_x_sav = 0;
uint32_t cur_y_sav = 0;

bool intensity;

extern int printf(const char*, ...);

uint8_t vga_colors_default[] = {
	VGA_COLOR_BLACK,
	VGA_COLOR_RED,
	VGA_COLOR_GREEN,
	VGA_COLOR_BROWN,
	VGA_COLOR_BLUE,
	VGA_COLOR_MAGENTA,
	VGA_COLOR_DARK_GREY,
};

uint8_t vga_colors_intense[] = {
	VGA_COLOR_LIGHT_GREY,
	VGA_COLOR_LIGHT_RED,
	VGA_COLOR_LIGHT_GREEN,
	VGA_COLOR_LIGHT_BROWN,
	VGA_COLOR_LIGHT_BLUE,
	VGA_COLOR_LIGHT_MAGENTA,
	VGA_COLOR_LIGHT_CYAN,
	VGA_COLOR_WHITE,
};

void vga_text_write(const char* data, size_t size) {
	
	ansi_seq = data[0] == 0x1B;

	if (!ansi_seq) {
		for (size_t i = 0; i < size; i++)
			vga_text_putchar(data[i]);
	} else {
		for (size_t i = 1; i < size; i++) {
			if(i == 1) {
				if (data[i] == 'c') {	// reset
					terminal_row = 0;
					terminal_column = 0;
					terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

					for (size_t y = 0; y < VGA_HEIGHT; y++) {
						for (size_t x = 0; x < VGA_WIDTH; x++) {
							const size_t index = y * VGA_WIDTH + x;
							terminal_buffer[index] = vga_entry(' ', terminal_color);
						}
					}


					break;
				}

				if (data[i] == '[') {
					uint32_t leftover = size - i;
					if (leftover == 2) {	// we should use defaults for the command
						i++;
						switch(data[i]) {
							case 'A':
							case 'B':
							case 'C':
							case 'D':
							case 'E':
							case 'F':
							case 'G':
							case 'S':
							case 'T':
							case 'm': {
								csi_nums[0] = 1;
								break;
							}

							case 'J':
							case 'K': {
								csi_nums[0] = 0;
								break;
							}

							case 'H':
							case 'f': {
								csi_nums[0] = 1;
								csi_nums[1] = 1;
								break;
							}

						}
					} else {
						csi_nums_idx = 0;
						while (true) {
							i++;
							csi_num_idx = 0;
							
							while (is_digit(data[i])) {
								csi_num[csi_num_idx++] = data[i++];
								//terminal_buffer[0] = vga_entry(data[i++], 0x07);
								
							}
							csi_num[csi_num_idx] = '\0';
							csi_nums[csi_nums_idx++] = k_atoi(csi_num);
							
							if (data[i] == ';')
								continue;
							else
								break;
						}
					}

					switch (data[i]) {	// actual functions
						case 'A': {
							terminal_row -= csi_nums[0]; 
							break;
						}

						case 'B': {
							terminal_row += csi_nums[0]; 
							break;
						}

						case 'C': {
							terminal_column += csi_nums[0]; 
							break;
						}

						case 'D': {
							terminal_column -= csi_nums[0]; 
							break;
						}

						case 'E': {
							terminal_row = 0;
							terminal_column += csi_nums[0]; 
							break;
						}

						case 'F': {
							terminal_row = 0;
							terminal_column -= csi_nums[0]; 
							break;
						}

						case 'G': {
							terminal_column = csi_nums[0]; 
							break;
						}

						case 'f':
						case 'H': {
							terminal_row = csi_nums[0] - 1; 
							terminal_column = csi_nums[1] - 1; 
							break;
						}

						case 's': {
							cur_x_sav = terminal_column;
							cur_y_sav = terminal_row;
							break;
						}

						case 'u': {
							terminal_column = cur_x_sav;
							terminal_row = cur_y_sav;
							break;
						}

						case 'm': {

							switch(csi_nums[0]) {
								case 1:
									intensity = 1;
									break;
								case 0:
									intensity = 0;
									break;

								case 30:
								case 31:
								case 32:
								case 33:
								case 34:
								case 35:
								case 36:
								case 37: {
									uint32_t col = csi_nums[0] - 30;
									if (!intensity) {
										fg = vga_colors_default[col];
									} else {
										fg = vga_colors_intense[col];
									}
									break;
								}

								case 40:
								case 41:
								case 42:
								case 43:
								case 44:
								case 45:
								case 46:
								case 47: {
									uint32_t col = csi_nums[0] - 40;
									if (!intensity) {
										bg = vga_colors_default[col];
									} else {
										bg = vga_colors_intense[col];
									}
									break;
								}
									
							}

							terminal_color = vga_entry_color((vga_color)fg, (vga_color)bg);

							break;
						}

					}
				}
			}
		}
	}

	ansi_seq = false;
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
