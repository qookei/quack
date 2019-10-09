#include "gdt.h"

#include <mm/pmm.h>
#include <mm/mm.h>
#include <string.h>
#include <stddef.h>

// -------------------- TSS --------------------

struct tss {
	uint32_t unused1;
	uint64_t rsp[4];
	uint64_t unused2;
	uint64_t ist[7];
	uint64_t unused3;
	uint16_t unused4;
	uint16_t io_bitmap_offset;
	uint8_t io_bitmap[8193]; // (65536 / 8) + terminator byte
} __attribute__((packed));

void *tss_create_new(void) {
	uintptr_t phys = (uintptr_t)pmm_alloc((sizeof(struct tss) + PAGE_SIZE - 1) / PAGE_SIZE);
	struct tss *t = (struct tss *)(phys + VIRT_PHYS_BASE);
	memset(t, 0, sizeof(struct tss));
	return t;
}

void tss_update_stack(void *tss, uintptr_t stack, int ring) {
	struct tss *t = tss;
	t->rsp[ring] = stack;
}

void tss_update_io_bitmap(void *tss, void *bitmap) {
	struct tss *t = tss;
	t->io_bitmap_offset = (uintptr_t)&t->io_bitmap - (uintptr_t)tss;
	memcpy(t->io_bitmap, bitmap, 8192);
	t->io_bitmap[8192] = 0xFF;
}

// -------------------- GDT --------------------

#define SEGMENT_COUNT 6 // NULL, Kern CODE64, User CODE64, User DATA, User TSS (2 entries)

static void create_null_seg(uint32_t *gdt, int idx) {
	gdt[idx * 2] = 0;
	gdt[idx * 2 + 1] = 0;
}

static void create_kern_code64_seg(uint32_t *gdt, int idx) {
	gdt[idx * 2] = 0;
	gdt[idx * 2 + 1] = 0xa09800;
}

static void create_user_code64_seg(uint32_t *gdt, int idx) {
	gdt[idx * 2] = 0;
	gdt[idx * 2 + 1] = 0xa0f800;
}

static void create_user_data_seg(uint32_t *gdt, int idx) {
	gdt[idx * 2] = 0;
	gdt[idx * 2 + 1] = 0xa0f200;
}

static void create_user_tss_seg(uint32_t *gdt, uintptr_t tss, int idx) {
	size_t size = sizeof(struct tss);

	gdt[idx * 2 + 0] = (size & 0xFFFF) | ((tss & 0xFFFF) << 16);
	gdt[idx * 2 + 1] = ((tss >> 16) & 0xFF) | 0x8900
			| (size & 0x000F0000) | (tss & 0xFF000000);
	gdt[idx * 2 + 2] = tss >> 32;
	gdt[idx * 2 + 3] = 0;
}

void *gdt_create_new(void *tss) {

	uintptr_t phys = (uintptr_t)pmm_alloc((sizeof(uint64_t) * SEGMENT_COUNT
				+ PAGE_SIZE - 1) / PAGE_SIZE);
	void *gdt = (void *)(phys + VIRT_PHYS_BASE);

	int i = 0;
	create_null_seg(gdt, i++);
	create_kern_code64_seg(gdt, i++);
	create_user_code64_seg(gdt, i++);
	create_user_data_seg(gdt, i++);
	create_user_tss_seg(gdt, (uintptr_t)tss, i++);
	i++; // account for extra used up slot

	return gdt;
}

void gdt_load(void *gdt) {
	struct gdtr {
		uint16_t len;
		uint64_t ptr;
	} __attribute__((packed)) gdtr;

	gdtr.len = SEGMENT_COUNT * 8 - 1;
	gdtr.ptr = (uint64_t)gdt;

	asm volatile ("lgdt %0" : : "m"(gdtr));
}

void gdt_load_tss(uint16_t sel) {
	asm volatile ("ltr %0" : : "r"(sel));
}

void gdt_load_gs_base(uintptr_t base) {
	asm volatile ("wrmsr" : : "a"((uint32_t)base), "d"((uint32_t)(base >> 32)), "c"(0xC0000101));
}

uintptr_t gdt_get_gs_base(void) {
	uint32_t hi, lo;
	asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000101));
	return ((uintptr_t)hi << 32) | (uintptr_t)lo;
}

void gdt_load_fs_base(uintptr_t base) {
	asm volatile ("wrmsr" : : "a"((uint32_t)base), "d"((uint32_t)(base >> 32)), "c"(0xC0000100));
}

uintptr_t gdt_get_fs_base(void) {
	uint32_t hi, lo;
	asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000100));
	return ((uintptr_t)hi << 32) | (uintptr_t)lo;
}
