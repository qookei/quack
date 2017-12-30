#include "vesa_text.h"
#include "vesa_font.h"

#include "../../kheap/heap.h"


#define VESA_BG 0x20

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
	
	return -1;
}

uint32_t cur_x = 0, cur_y = 0;
uint32_t cur_max_x, cur_max_y;

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
							uint16_t col = rgb888_rgb565(VESA_BG, VESA_BG, VESA_BG);
							vesa_bbuf[poff + 1] = col >> 8;
							vesa_bbuf[poff] = col & 0xFF;
							vesa_vbuf[poff + 1] = col >> 8;
							vesa_vbuf[poff] = col & 0xFF;
							break;
						}
						
						case 32:
						case 24: {
							vesa_bbuf[poff] = VESA_BG;
							vesa_bbuf[poff + 1] = VESA_BG;
							vesa_bbuf[poff + 2] = VESA_BG;
							vesa_vbuf[poff] = VESA_BG;
							vesa_vbuf[poff + 1] = VESA_BG;
							vesa_vbuf[poff + 2] = VESA_BG;
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

	uint8_t b = 32;

	for(uint32_t y = 0; y < mboot->framebuffer_height; y++) {
		for(uint32_t x = 0; x < mboot->framebuffer_width; x++) {
			uint32_t xx = x * mboot->framebuffer_bpp / 8;

			switch(mboot->framebuffer_bpp){
				case 32:
				case 24:
					vesa_vbuf[xx + y * mboot->framebuffer_pitch] = b;
					vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = b;
					vesa_vbuf[xx + y * mboot->framebuffer_pitch+2] = b;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch] = b;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = b;
					vesa_bbuf[xx + y * mboot->framebuffer_pitch+2] = b;
					break;
				case 16:
					uint16_t u = rgb888_rgb565(b,b,b);
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

}

extern void* memcpy(void* dst, const void* src, size_t len);
extern void* memmove(void* dst, const void* src, size_t len);

void vesa_scroll_up2(uint8_t lines) {
	
	for (uint32_t i = lines; i < mboot->framebuffer_height; i++)
		memmove(vesa_bbuf + (i - lines) * mboot->framebuffer_pitch, 
			    vesa_bbuf + i * mboot->framebuffer_pitch, 
			    mboot->framebuffer_width * mboot->framebuffer_bpp / 8);

	for (uint32_t y = 0; y < lines; y++) {
		for (uint32_t x = 0; x < mboot->framebuffer_width; x++) {
			uint32_t xx = x * mboot->framebuffer_bpp / 8;
			uint32_t yy = mboot->framebuffer_height - y;
			kprintf("%u %u\n", x, yy);

			switch(mboot->framebuffer_bpp){
				case 32:
				case 24:
					vesa_bbuf[xx + yy * mboot->framebuffer_pitch] = VESA_BG;
					vesa_bbuf[xx + yy * mboot->framebuffer_pitch+1] = VESA_BG;
					vesa_bbuf[xx + yy * mboot->framebuffer_pitch+2] = VESA_BG;
					break;
				case 16:
					uint16_t u = rgb888_rgb565(VESA_BG, VESA_BG, VESA_BG);
					vesa_bbuf[xx + yy * mboot->framebuffer_pitch] = u & 0xFF;
					vesa_bbuf[xx + yy * mboot->framebuffer_pitch+1] = u >> 8;
			}
		}
	}
	
	
	memcpy(vesa_vbuf, vesa_bbuf, mboot->framebuffer_height * mboot->framebuffer_pitch);
}

void vesa_scroll_up(uint8_t lines) {
	uint32_t pitch = lines * mboot->framebuffer_pitch;
	
	uint32_t bpp = mboot->framebuffer_bpp / 8;
	
	for (uint32_t i = lines; i < mboot->framebuffer_height; i++) {
		memcpy(vesa_bbuf + (i - lines) * mboot->framebuffer_pitch, vesa_bbuf + (i) * mboot->framebuffer_pitch, mboot->framebuffer_width * bpp);
	}
	
	memcpy(vesa_vbuf, vesa_bbuf, mboot->framebuffer_height * mboot->framebuffer_pitch);
	
	for (uint32_t vert = mboot->framebuffer_height * mboot->framebuffer_pitch - pitch; vert < mboot->framebuffer_height * mboot->framebuffer_pitch; vert += mboot->framebuffer_pitch) {
		for (uint32_t x = 0; x < mboot->framebuffer_width; x++) {
			
			switch (bpp) {
				case 2: {
					uint16_t col = rgb888_rgb565(VESA_BG, VESA_BG, VESA_BG);
					vesa_vbuf[vert + x * bpp] = col >> 8;
					vesa_vbuf[vert + x * bpp + 1] = col & 0xFF;
					vesa_bbuf[vert + x * bpp] = col >> 8;
					vesa_bbuf[vert + x * bpp + 1] = col & 0xFF;
					break;
				}
				
				case 4:
				case 3: {
					vesa_vbuf[vert + x * bpp] = VESA_BG;
					vesa_vbuf[vert + x * bpp + 1] = VESA_BG;
					vesa_vbuf[vert + x * bpp + 2] = VESA_BG;
					vesa_bbuf[vert + x * bpp] = VESA_BG;
					vesa_bbuf[vert + x * bpp + 1] = VESA_BG;
					vesa_bbuf[vert + x * bpp + 2] = VESA_BG;
					break;
				}
			}
			
		}
	
	}
	
	
}

void vesa_text_putchar(char c) {
	if (c == '\n') {
		cur_x = 0;

		if (++cur_y == cur_max_y) {
			cur_y = cur_max_y-1;
			vesa_scroll_up(font.Height);
		}

		return;
	}

	if (c == '\r') {
		cur_x = 0;
		return;
	}

	if (c == '\b') {
		if (!cur_x) {
			cur_x = cur_max_x - 1;
			if (!(--cur_y)) cur_y = 0;
		} else {
			cur_x--;
		}
		

		vesa_putchar(' ', cur_x * font.Width, cur_y * font.Height, 0xFF, 0xFF, 0xFF);
		return;
	}


	vesa_putchar(c, cur_x * font.Width, cur_y * font.Height, 0xFF, 0xFF, 0xFF);
	if (++cur_x == cur_max_x) {
		cur_x = 0;
		if (++cur_y == cur_max_y){
			cur_y = cur_max_y-1;
			vesa_scroll_up(font.Height);
		}
	}
}


extern int k_atoi(char *p);

extern bool ansi_seq;

extern char csi_num[32];
extern uint32_t csi_nums[32];

extern uint8_t csi_nums_idx;
extern uint8_t csi_num_idx;

extern uint32_t cur_x_sav;
extern uint32_t cur_y_sav;

extern int printf(const char*, ...);

#define is_digit(c) (c >= '0' && c <= '9')

void vesa_text_write(const char* data, size_t size) {
	
	ansi_seq = data[0] == 0x1B;

	if (!ansi_seq) {
		for (size_t i = 0; i < size; i++)
			vesa_text_putchar(data[i]);
	} else {
		for (size_t i = 1; i < size; i++) {
			if(i == 1) {
				if (data[i] == 'c') {	// reset

					uint8_t b = 32;

					for(uint32_t y = 0; y < mboot->framebuffer_height; y++) {
						for(uint32_t x = 0; x < mboot->framebuffer_width; x++) {
							uint32_t xx = x * mboot->framebuffer_bpp / 8;

							switch(mboot->framebuffer_bpp){
								case 32:
								case 24:
									vesa_vbuf[xx + y * mboot->framebuffer_pitch] = b;
									vesa_vbuf[xx + y * mboot->framebuffer_pitch+1] = b;
									vesa_vbuf[xx + y * mboot->framebuffer_pitch+2] = b;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch] = b;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch+1] = b;
									vesa_bbuf[xx + y * mboot->framebuffer_pitch+2] = b;
									break;
								case 16:
									uint16_t u = rgb888_rgb565(b,b,b);
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
							cur_y -= csi_nums[0]; 
							break;
						}

						case 'B': {
							cur_y += csi_nums[0]; 
							break;
						}

						case 'C': {
							cur_x += csi_nums[0]; 
							break;
						}

						case 'D': {
							cur_x -= csi_nums[0]; 
							break;
						}

						case 'E': {
							cur_y = 0;
							cur_x += csi_nums[0]; 
							break;
						}

						case 'F': {
							cur_y = 0;
							cur_x -= csi_nums[0]; 
							break;
						}

						case 'G': {
							cur_x = csi_nums[0]; 
							break;
						}

						case 'f':
						case 'H': {
							cur_x = csi_nums[0] - 1; 
							cur_y = csi_nums[1] - 1; 
							break;
						}


						case 's': {
							cur_x_sav = cur_x;
							cur_y_sav = cur_y;
							break;
						}

						case 'u': {
							cur_x = cur_x_sav;
							cur_y = cur_y_sav;
							break;
						}

						case 'm': {

							switch(csi_nums[0]) {
								
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