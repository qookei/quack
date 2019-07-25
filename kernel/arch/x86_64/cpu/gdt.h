#ifndef GDT_T
#define GDT_H

#include <stdint.h>

#define GDT_TSS_SEL 0x20

void *tss_create_new(void);
void tss_update_stack(void *tss, uintptr_t stack, int ring);
void tss_update_io_bitmap(void *tss, void *bitmap);

void *gdt_create_new(void *tss);
void gdt_load(void *gdt);
void gdt_load_tss(uint16_t);

void gdt_load_gs_base(uintptr_t base);
uintptr_t gdt_get_gs_base(void);
void gdt_load_fs_base(uintptr_t base);
uintptr_t gdt_get_fs_base(void);

#endif
