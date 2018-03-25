#ifndef TTY_BACKEND_VESA
#define TTY_BACKEND_VESA

#include <tty/tty.h>
#include <multiboot.h>
#include <paging/paging.h>


void vesa_scroll_up(uint8_t lines);
void vesa_putchar(uint16_t c, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);

void vesa_text_tty_set_mboot(multiboot_info_t *);
void vesa_text_tty_dev(tty_op,uint32_t);

#endif