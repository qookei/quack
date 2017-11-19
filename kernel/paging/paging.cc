#include "paging.h"

// uint32_t dir[1024] __attribute__((aligned(4096))) = {0x00000000};

//page_dir_internal dir_int;
page_directory dir_;

uint32_t *page_dir_table;

uint32_t tab[1024] __attribute__((aligned(4096)));

extern int printf(const char*, ...);
extern void *memcpy(void *, const void *, size_t);

void paging_init(void) {

	for(int i = 0; i < 1024; i++) {
		tab[1023 - i] = (uint32_t)pmm_alloc() | 0x3;
	}

	uint32_t kernel_addr = (0xC0000000 >> 22);

	int i = 0;
	
	uint32_t tabaddr = (uint32_t)tab;
 
	printf("tab = %08x\n", tabaddr);
	printf("tab = %08x\n", tabaddr -= 0xC0000000);
	printf("tab = %08x\n", tabaddr | 0x83);
	printf("tab[0] = %08x\n", tab[0]);

	dir_.entries[i++] = (page_table *)(tabaddr | 0x3);
	dir_.entries[i++] = (page_table *)0x0;
	
	for (int j = 0; j < kernel_addr - 2; j++)
		dir_.entries[i++] = (page_table *)0x0;
	
	dir_.entries[i++] = (page_table *)0x00000083;
	dir_.entries[i++] = (page_table *)0x00400083;
	
	for (int j = 0; j < 1024 - kernel_addr - 2; j++)
		dir_.entries[i++] = (page_table *)0x0;
	
	uint32_t addr = (uint32_t)&dir_.entries - 0xC0000000;

	// now we can do stuff

	asm volatile("mov %0, %%eax\nmov %%eax, %%cr3" : : "r"(addr) : "%eax");

	memcpy(0x0, dir_.entries, 4096);

	printf("%08x\n", tab[0] & 0xFFFFF000);

	asm volatile("mov %0, %%eax\nmov %%eax, %%cr3" : : "r"(tab[0] & 0xFFFFF000) : "%eax");

	printf("paging init\n");
}