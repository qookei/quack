#include "paging.h"

page_directory dir_;

extern int kprintf(const char*, ...);
extern void *memcpy(void *, const void *, size_t);

uint32_t current_pd;

void set_cr3(uint32_t addr) {
	current_pd = addr;
	asm volatile("mov %0, %%eax\nmov %%eax, %%cr3" : : "r"(addr) : "%eax");
}

uint32_t get_cr3() {
	uint32_t a;
	asm volatile("mov %%cr3, %0" : "=r"(a) : : );
	return a;
	// return current_pd;
}

uint32_t def_cr3() {
	return (uint32_t)dir_.entries - 0xC0000000;
}

static inline void tlb_flush_entry(uint32_t addr) {
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}


uint32_t pt_f[1024] __attribute__((aligned(4096))) = {0};

uint32_t alloc_clean_page() {

    uint32_t *pd = (uint32_t *)0xFFFFF000;

	for(uint32_t i = 0; i < 1024; i++) pt_f[i] = 0;

	pt_f[0] = (uint32_t)pmm_alloc() | 0x3;
	pd[1023] = (((uint32_t)pt_f - 0xC0000000) | 0x3);

	tlb_flush_entry(0xFFC00000);

	uint8_t *pt = (uint8_t *)0xFFC00000;
	
	for(uint32_t i = 0; i < 4096; i++) pt[i] = 0;
	
	pd[1023] = ((current_pd) | 0x3);
	
	tlb_flush_entry(0xFFC00000);
	
	return pt_f[0] & 0xFFFFF000;
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
	if ((((uint32_t)physaddr) & 0xFFF) != 0 || (((uint32_t)virtualaddr)&0xFFF) != 0) {
		kprintf("map_page with unaligned address(es)!\n");
		return;
	}

	kprintf("cr3: %08x p: %08p v: %08p\n", get_cr3(), physaddr, virtualaddr);
 
    uint32_t pdindex = (uint32_t)virtualaddr >> 22;
    uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

    uint32_t *pd = (uint32_t *)0xFFFFF000;
    if (!(pd[pdindex] & 0x1)) pd[pdindex] = alloc_clean_page() | (flags & 0xFFF);
	

    uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * pdindex);

    if(pt[ptindex] & 0x1) {
 		kprintf("map_page on already mapped address! %08p %08p\n", physaddr, virtualaddr);
		return;
    }
    
    pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | 0x01; // Present

    tlb_flush_entry((uint32_t)virtualaddr);
}

void unmap_page(void *virtualaddr) {
	if ((((uint32_t)virtualaddr)&0xFFF) != 0) {
		kprintf("unmap_page with unaligned address!\n");
		return;
	}
 
    uint32_t pdindex = (uint32_t)virtualaddr >> 22;
    uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

    uint32_t *pd = (uint32_t *)0xFFFFF000;
    if (!(pd[pdindex] & 0x1)) {
    	kprintf("unmap_page on nonexistent page directory entry!\n");
	}

    uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * pdindex);
    pt[ptindex] = 0x00000000;

    tlb_flush_entry((uint32_t)virtualaddr);
}

void *get_phys(void * virtualaddr) {
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;
 
    unsigned long * pd = (unsigned long *)0xFFFFF000;
    if (!(pd[pdindex] & 0x1)) {
    	kprintf("get_phys on unmapped address");
    	return NULL;
	}
 
    unsigned long * pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
    if (!(pt[ptindex] & 0x1)) {
    	kprintf("get_phys on unmapped address");
    	return NULL;
    }
 
    return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

uint32_t create_page_directory(multiboot_info_t* mboot) {
	uint32_t kernel_addr = (0xC0000000 >> 22);

	uint32_t i = 0;

	uint32_t tmp_addr = (uint32_t)pmm_alloc();
	map_page((void *)tmp_addr, (void*)0xE0000000, 0x3);

	uint32_t addr = 0xE0000000;
	uint32_t *new_dir = (uint32_t *)addr;

	for (uint32_t j = 0; j < kernel_addr; j++)
		new_dir[i++] = 0x0;
	
	new_dir[i++] = 0x00000083;
	new_dir[i++] = 0x00400083;
	new_dir[i++] = 0x00800083;
	
	for (uint32_t j = 0; j < 1024 - kernel_addr - 4; j++)
		new_dir[i++] = 0x0;

	new_dir[i++] = (tmp_addr | 0x3);

	unmap_page((void*)0xE0000000);

	return tmp_addr;

}

void destroy_page_directory(void *pd) {
	if (get_cr3() == (uint32_t)pd) {
		kprintf("destroy_page_directory on used page directory!\n");
		return;
	}

	pmm_free(pd);
}

void paging_init(void) {

	uint32_t kernel_addr = (0xC0000000 >> 22);

	uint32_t i = 0;

	uint32_t addr = (uint32_t)&dir_.entries - 0xC0000000;

	dir_.entries[i++] = (uint32_t *)0x0;
	dir_.entries[i++] = (uint32_t *)0x0;
	dir_.entries[i++] = (uint32_t *)0x0;
	
	for (uint32_t j = 0; j < kernel_addr - 3; j++)
		dir_.entries[i++] = (uint32_t *)0x0;
	
	dir_.entries[i++] = (uint32_t *)0x00000083;
	dir_.entries[i++] = (uint32_t *)0x00400083;
	dir_.entries[i++] = (uint32_t *)0x00800083;
	
	for (uint32_t j = 0; j < 1024 - kernel_addr - 4; j++)
		dir_.entries[i++] = (uint32_t *)0x0;

	dir_.entries[i++] = (uint32_t *)(addr | 0x3);

	set_cr3(addr);

	kprintf("init cr3: %08x\n", get_cr3());


	kprintf("[kernel] paging ok\n");

}