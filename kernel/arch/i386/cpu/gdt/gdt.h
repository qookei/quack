#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

void gdt_set_tss_stack(uint32_t stack);
void gdt_load_io_bitmap(uint8_t *io_bitmap);
void gdt_setup();

#endif
