#include "vesa_text.h"
#include "vesa_font.h"

#include <kheap/liballoc.h>
#include <ctype.h>
#include <stdlib.h>

uint8_t text_R = 0xAA;
uint8_t text_G = 0xAA;
uint8_t text_B = 0xAA;

uint8_t VESA_BG_R = 0x00;
uint8_t VESA_BG_G = 0x00;
uint8_t VESA_BG_B = 0x00;

multiboot_info_t* mboot;
uint8_t* vesa_vbuf;
uint8_t* vesa_bbuf;

void vesa_text_tty_set_mboot(multiboot_info_t * hdr) {
	mboot = hdr;
}

void vesa_scroll_up(uint8_t lines);


extern int kprintf(const char *fmt, ...);

uint16_t rgb888_rgb565(uint8_t r, uint8_t g, uint8_t b) {
	uint8_t _r = r >> (8 - 5);
	uint8_t _g = g >> (8 - 6);
	uint8_t _b = b >> (8 - 5);
	return (_r << 11) | (_g << 5) | (_b);
}

uint32_t findIndexOf(uint16_t what, const uint16_t* where, uint32_t howMuchWhere) {
	uint32_t index;
	for (index = 0; index < howMuchWhere; index++)
		if (where[index] == what) return index;
	
	return 0;
}

uint32_t cur_x = 0, cur_y = 0;
uint32_t cur_max_x, cur_max_y;

void vesa_ppx(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
	uint32_t pitch = y * mboot->framebuffer_pitch;
	uint32_t bpp = mboot->framebuffer_bpp / 8;
	
	uint32_t poff = x * bpp + pitch;

	vesa_vbuf[poff] = r;
	vesa_vbuf[poff + 1] = g;
	vesa_vbuf[poff + 2] = b;
}

void vesa_bgppx(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
	uint32_t pitch = y * mboot->framebuffer_pitch;
	uint32_t bpp = mboot->framebuffer_bpp / 8;
	
	uint32_t poff = x * bpp + pitch;

	vesa_bbuf[poff] = r;
	vesa_bbuf[poff + 1] = g;
	vesa_bbuf[poff + 2] = b;
}

void vesa_resetppx(uint32_t x, uint32_t y) {
	uint32_t pitch = y * mboot->framebuffer_pitch;
	uint32_t bpp = mboot->framebuffer_bpp / 8;
	
	uint32_t poff = x * bpp + pitch;

	vesa_vbuf[poff] = vesa_bbuf[poff];
	vesa_vbuf[poff + 1] = vesa_bbuf[poff + 1];
	vesa_vbuf[poff + 2] = vesa_bbuf[poff + 2];
}

void vesa_putchar(uint16_t c, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
	
	uint32_t pitch = y * mboot->framebuffer_pitch;
	
	uint32_t bpp = mboot->framebuffer_bpp / 8;
	
	if (1) {
		// valid
		
		uint32_t idx = findIndexOf(c, font.Index, font.Chars);
		
		if (idx == (uint32_t)(0-1))
			return;
		
		uint32_t bit_start_idx = font.Height * idx;
		
		
		
		for (uint32_t y = pitch; y < pitch + font.Height * mboot->framebuffer_pitch; y += mboot->framebuffer_pitch) {
			uint16_t u = 0;
			
			if (font.Width == 16) {
				
				uint32_t idx = bit_start_idx + (y - pitch) / mboot->framebuffer_pitch;
				
				u = font.Bitmap[idx * 2];
				u |= font.Bitmap[idx * 2 + 1] << 16;
				
				
			} else if (font.Width == 8) {
				u = font.Bitmap[bit_start_idx + (y - pitch) / mboot->framebuffer_pitch];
			}
			
			for (uint32_t xx = 0; xx < font.Width; xx++) {
				uint32_t poff = y + (x + xx) * bpp;
				uint8_t bit = (u >> (font.Width - xx - 1)) & 0x1;
				
				if (bit) {
					switch(mboot->framebuffer_bpp) {
						case 16: {
							uint16_t col = rgb888_rgb565(r, g, b);
							vesa_bbuf[poff + 1] = col >> 8;
							vesa_bbuf[poff] = col & 0xFF;
							vesa_vbuf[poff + 1] = col >> 8;
							vesa_vbuf[poff] = col & 0xFF;
							break;
						}
						
						case 32:
						case 24: {
							vesa_bbuf[poff] = b;
							vesa_bbuf[poff + 1] = g;
							vesa_bbuf[poff + 2] = r;
							vesa_vbuf[poff] = b;
							vesa_vbuf[poff + 1] = g;
							vesa_vbuf[poff + 2] = r;
							break;
						}
					}
				} else {
					switch(mboot->framebuffer_bpp) {
						case 16: {
							uint16_t col = rgb888_rgb565(VESA_BG_R, VESA_BG_G, VESA_BG_B);
							vesa_bbuf[poff + 1] = col >> 8;
							vesa_bbuf[poff] = col & 0xFF;
							vesa_vbuf[poff + 1] = col >> 8;
							vesa_vbuf[poff] = col & 0xFF;
							break;
						}
						
						case 32:
						case 24: {
							vesa_bbuf[poff] = VESA_BG_R;
							vesa_bbuf[poff + 1] = VESA_BG_G;
							vesa_bbuf[poff + 2] = VESA_BG_B;
							vesa_vbuf[poff] = VESA_BG_R;
							vesa_vbuf[poff + 1] = VESA_BG_G;
							vesa_vbuf[poff + 2] = VESA_BG_B;
							break;
						}
					}
				}
			}
		}
		
	}
}


void vesa_text_init() {
	uint32_t addr = mboot->framebuffer_addr;
	addr &= 0xFFFFF000;
	
	uint32_t dst = 0xD0000000;
	uint32_t fbuf_size = mboot->framebuffer_pitch * mboot->framebuffer_height;
	
	uint32_t pages_to_map = fbuf_size / 4096 + 1;
	
	for (uint32_t i = 0; i < pages_to_map; i++) {
		map_page((void*)(addr), (void*)(dst), 3);
	
		addr += 0x1000;
		dst += 0x1000;

	}

	vesa_bbuf = (uint8_t *)kmalloc(mboot->framebuffer_pitch * mboot->framebuffer_height);

	uint32_t off = (uint32_t)mboot->framebuffer_addr & 0xFFF;	// since we ingored the lsb, get them and add them to new address
	vesa_vbuf = (uint8_t*)(0xD0000000 + off);

	for(uint32_t y = 0; y < mboot->framebuffer_height; y++) {
		for(uint32_t x = 0; x < mboot->framebuffer_width; x++) {
			uint32_t xx = x * mboot->framebuffer_bpp / 8;

			switch(mboot->framebuffer_bpp){
				case 32:
				case 24:
					vesa_vbuf[xx + y * mboot->framebuffer_pitch] = VESA_BG_R;
					vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = VESA_BG_G;
					vesa_vbuf[xx + y * mboot->framebuffer_pitch+2] = VESA_BG_B;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch] = VESA_BG_R;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = VESA_BG_G;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch+2] = VESA_BG_B;
					break;
				case 16:
					uint16_t u = rgb888_rgb565(VESA_BG_R,VESA_BG_G,VESA_BG_B);
					vesa_vbuf[xx + y * mboot->framebuffer_pitch] = u & 0xFF;
					vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = u >> 8;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch] = u & 0xFF;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = u >> 8;
			}
		}
	}

	cur_max_x = mboot->framebuffer_width / font.Width;
	cur_max_y = mboot->framebuffer_height / font.Height;
	kprintf("%u %u\n", cur_max_x, cur_max_y);
	kprintf("%u\n", mboot->framebuffer_pitch * mboot->framebuffer_height);

}

extern void* memcpy(void* dst, const void* src, size_t len);
extern void* memmove(void* dst, const void* src, size_t len);

void vesa_scroll_up(uint8_t lines) {
	uint32_t pitch = lines * mboot->framebuffer_pitch;
	
	uint32_t bpp = mboot->framebuffer_bpp / 8;
	
	for (uint32_t i = lines; i < mboot->framebuffer_height; i++) {
		memcpy((vesa_bbuf + (i - lines) * mboot->framebuffer_pitch), (vesa_bbuf + (i) * mboot->framebuffer_pitch), mboot->framebuffer_width * bpp);
	}
	
	memcpy(vesa_vbuf, vesa_bbuf, mboot->framebuffer_height * mboot->framebuffer_pitch);
	
	for (uint32_t vert = mboot->framebuffer_height * mboot->framebuffer_pitch - pitch; vert < mboot->framebuffer_height * mboot->framebuffer_pitch; vert += mboot->framebuffer_pitch) {
		for (uint32_t x = 0; x < mboot->framebuffer_width; x++) {
			
			switch (bpp) {
				case 2: {
					uint16_t col = rgb888_rgb565(VESA_BG_R, VESA_BG_G, VESA_BG_B);
					vesa_vbuf[vert + x * bpp] = col >> 8;
					vesa_vbuf[vert + x * bpp + 1] = col & 0xFF;
					vesa_bbuf[vert + x * bpp] = col >> 8;
					vesa_bbuf[vert + x * bpp + 1] = col & 0xFF;
					break;
				}
				
				case 4:
				case 3: {
					vesa_vbuf[vert + x * bpp] = VESA_BG_R;
					vesa_vbuf[vert + x * bpp + 1] = VESA_BG_G;
					vesa_vbuf[vert + x * bpp + 2] = VESA_BG_B;
					vesa_bbuf[vert + x * bpp] = VESA_BG_R;
					vesa_bbuf[vert + x * bpp + 1] = VESA_BG_G;
					vesa_bbuf[vert + x * bpp + 2] = VESA_BG_B;
					break;
				}
			}
			
		}
	
	}
	
	
}

void vesa_text_putchar(char c) {
	if (c == '\n') {
		vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
		cur_x = 0;

		if (++cur_y == cur_max_y) {
			cur_y = cur_max_y-1;
			vesa_scroll_up(font.Height);
		}

		vesa_putchar(0x01, cur_x * font.Width, cur_y * font.Height, text_R, text_R, text_R);

		return;
	}

	if (c == '\r') {
		vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
		cur_x = 0;
		vesa_putchar(0x01, cur_x * font.Width, cur_y * font.Height, text_R, text_R, text_R);
		return;
	}

	if (c == '\b') {
		vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
		if (!cur_x) {
			cur_x = cur_max_x - 1;
			if (!(--cur_y)) cur_y = 0;
		} else {
			cur_x--;
		}
		
		vesa_putchar(0x01, cur_x * font.Width, cur_y * font.Height, text_R, text_R, text_R);
		return;
	}


	vesa_putchar(c, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);

	if (++cur_x == cur_max_x) {
		cur_x = 0;
		if (++cur_y == cur_max_y){
			cur_y = cur_max_y-1;
			vesa_scroll_up(font.Height);
		}
	}

	vesa_putchar(0x01, cur_x * font.Width, cur_y * font.Height, text_R, text_R, text_R);
	
}


int k_atoi(char *p);

bool ansi_seq;

char csi_num[32];
uint32_t csi_nums[32];

uint8_t csi_nums_idx;
uint8_t csi_num_idx;

uint32_t cur_x_sav;
uint32_t cur_y_sav;

int printf(const char*, ...);

bool inc_intensity = 0;

uint8_t colors_default[] = {
	0,   0,     0,
	170, 0,     0,
	0,   170,   0,
	170, 85,    0,
	0,   0,   170,
	170, 0,   170,
	0,   170, 170,
	170, 170, 170,
};

uint8_t colors_intense[] = {
	85,  85,   85,
	255, 85,   85,
	85,  255,  85,
	255, 255,  85,
	85,  85,  255,
	255, 85,  255,
	85,  255, 255,
	255, 255, 255,
};

void vesa_text_write(const char* data, size_t size) {
	
	ansi_seq = data[0] == 0x1B;

	if (!ansi_seq) {
		for (size_t i = 0; i < size; i++)
			vesa_text_putchar(data[i]);
	} else {
		for (size_t i = 1; i < size; i++) {
			if(i == 1) {
				if (data[i] == 'c') {	// reset

					for(uint32_t y = 0; y < mboot->framebuffer_height; y++) {
						for(uint32_t x = 0; x < mboot->framebuffer_width; x++) {
							uint32_t xx = x * mboot->framebuffer_bpp / 8;

							switch(mboot->framebuffer_bpp){
								case 32:
								case 24:
									vesa_vbuf[xx + y * mboot->framebuffer_pitch] = VESA_BG_R;
									vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = VESA_BG_G;
									vesa_vbuf[xx + y * mboot->framebuffer_pitch+2] = VESA_BG_B;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch] = VESA_BG_R;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = VESA_BG_G;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch+2] = VESA_BG_B;
									break;
								case 16:
									uint16_t u = rgb888_rgb565(VESA_BG_R,VESA_BG_G,VESA_BG_B);
									vesa_vbuf[xx + y * mboot->framebuffer_pitch] = u & 0xFF;
									vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = u >> 8;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch] = u & 0xFF;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = u >> 8;
							}
						}
					}

					cur_x = 0;
					cur_y = 0;
					
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
							
							while (isdigit(data[i])) {
								csi_num[csi_num_idx++] = data[i++];
								//terminal_buffer[0] = vga_entry(data[i++], 0x07);
								
							}
							csi_num[csi_num_idx] = '\0';
							csi_nums[csi_nums_idx++] = atoi(csi_num);
							
							if (data[i] == ';')
								continue;
							else
								break;
						}
					}

					switch (data[i]) {	// actual functions
						case 'A': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_y -= csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'J': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							
							for(uint32_t y = 0; y < mboot->framebuffer_height; y++) {
								for(uint32_t x = 0; x < mboot->framebuffer_width; x++) {
									uint32_t xx = x * mboot->framebuffer_bpp / 8;

									switch(mboot->framebuffer_bpp){
										case 32:
										case 24:
											vesa_vbuf[xx + y * mboot->framebuffer_pitch] = VESA_BG_R;
											vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = VESA_BG_G;
											vesa_vbuf[xx + y * mboot->framebuffer_pitch+2] = VESA_BG_B;
											vesa_bbuf[xx + y * mboot->framebuffer_pitch] = VESA_BG_R;
											vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = VESA_BG_G;
											vesa_bbuf[xx + y * mboot->framebuffer_pitch+2] = VESA_BG_B;
											break;
										case 16:
											uint16_t u = rgb888_rgb565(VESA_BG_R,VESA_BG_G,VESA_BG_B);
											vesa_vbuf[xx + y * mboot->framebuffer_pitch] = u & 0xFF;
											vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = u >> 8;
											vesa_bbuf[xx + y * mboot->framebuffer_pitch] = u & 0xFF;
											vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = u >> 8;
									}
								}
							}

							cur_x = 0;
							cur_y = 0;

							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'B': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_y += csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'C': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_x += csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'D': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_x -= csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'E': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_y = 0;
							cur_x += csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'F': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_y = 0;
							cur_x -= csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'G': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_x = csi_nums[0]; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'f':
						case 'H': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_x = csi_nums[0] - 1; 
							cur_y = csi_nums[1] - 1; 
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}


						case 's': {
							cur_x_sav = cur_x;
							cur_y_sav = cur_y;
							break;
						}

						case 'u': {
							vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							cur_x = cur_x_sav;
							cur_y = cur_y_sav;
							vesa_putchar(0x1, cur_x * font.Width, cur_y * font.Height, text_R, text_G, text_B);
							break;
						}

						case 'm': {

							switch(csi_nums[0]) {
								case 1: inc_intensity = 1; break;
								case 0: inc_intensity = 0; break;

								case 30:
								case 31:
								case 32:
								case 33:
								case 34:
								case 35:
								case 36:
								case 37: {
									uint32_t col = csi_nums[0] - 30;
									if (!inc_intensity) {
										text_R = colors_default[col * 3 + 0];
										text_G = colors_default[col * 3 + 1];
										text_B = colors_default[col * 3 + 2];
									} else {
										text_R = colors_intense[col * 3 + 0];
										text_G = colors_intense[col * 3 + 1];
										text_B = colors_intense[col * 3 + 2];
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
									if (!inc_intensity) {
										VESA_BG_R = colors_default[col * 3 + 0];
										VESA_BG_G = colors_default[col * 3 + 1];
										VESA_BG_B = colors_default[col * 3 + 2];
									} else {
										VESA_BG_R = colors_intense[col * 3 + 0];
										VESA_BG_G = colors_intense[col * 3 + 1];
										VESA_BG_B = colors_intense[col * 3 + 2];
									}
									break;
								}
								

							}

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

void vesa_text_writestring(const char* data) {
	vesa_text_write(data, strlen(data));
}

void vesa_text_tty_dev(tty_op op, uint32_t param) {

	switch(op) {
		case tty_op::INIT: {
			vesa_text_init();
			break;
		}

		case tty_op::WRITE_CHAR: {
			vesa_text_putchar((char)param);
			break;
		}

		case tty_op::WRITE_STRING: {
			vesa_text_writestring((const char*)param);
			break;
		}

	}

}
