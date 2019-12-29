#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define GDT_CODE_KERNEL 0xC09A
#define GDT_DATA_KERNEL 0xC092
#define GDT_CODE_USER 0xC0FA
#define GDT_DATA_USER 0xC0F2
#define GDT_TSS 0x4089

uint64_t create_descriptor(uint32_t base, uint32_t limit, uint16_t flag) {
	uint64_t descriptor;

	descriptor	=  limit	   & 0x000F0000;
	descriptor |= (flag <<	8) & 0x00F0FF00;
	descriptor |= (base >> 16) & 0x000000FF;
	descriptor |=  base		   & 0xFF000000;

	descriptor <<= 32;

	descriptor |= base	<< 16;
	descriptor |= limit  & 0x0000FFFF;

	return descriptor;
}

uint64_t gdt_arr[6] __attribute__((aligned(16))) = {0};

typedef struct {
	uint32_t prev_tss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed)) tss_t;

typedef struct {
	tss_t tss;
	uint8_t io_bitmap[8193];
} gdt_tss_data_t;

gdt_tss_data_t tss_data;

void gdt_set_tss_stack(uint32_t stack) {
	tss_data.tss.esp0 = stack;
}

void gdt_load_io_bitmap(uint8_t *io_bitmap) {
	memcpy(tss_data.io_bitmap, io_bitmap, 8192);
}

void gdt_setup() {
	gdt_arr[0] = create_descriptor(0, 0, 0);
	gdt_arr[1] = create_descriptor(0, 0x000FFFFF, GDT_CODE_KERNEL);
	gdt_arr[2] = create_descriptor(0, 0x000FFFFF, GDT_DATA_KERNEL);
	gdt_arr[3] = create_descriptor(0, 0x000FFFFF, GDT_CODE_USER);
	gdt_arr[4] = create_descriptor(0, 0x000FFFFF, GDT_DATA_USER);
	gdt_arr[5] = create_descriptor((uint32_t)&(tss_data.tss), sizeof(tss_data), GDT_TSS);

	memset(&tss_data, 0, sizeof(tss_data));
	tss_data.tss.ss0 = 0x10;
	tss_data.tss.iomap_base = (uintptr_t)tss_data.io_bitmap - (uintptr_t)&tss_data;
	tss_data.io_bitmap[8192] = 0xFF;

	asm volatile ("call gdt_load" : : "c"((uint32_t)gdt_arr), "d"((uint16_t)6*8) : "%ax");
}
