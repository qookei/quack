#include "paging.h"

// uint32_t dir[1024] __attribute__((aligned(4096))) = {0x00000000};

//page_dir_internal dir_int;
page_directory dir_;

uint32_t *page_dir_table;

extern int printf(const char*, ...);

void paging_init(void) {

	for(int i = 0; i < 1024; i++)
		page_dir_table = (uint32_t *)pmm_alloc();

	uint32_t kernel_addr = (0xC0000000 >> 22);

	int i = 0;
	
	dir_.entries[i++] = (page_table *)((uint32_t)page_dir_table | 0x83);
	dir_.entries[i++] = (page_table *)0x0;
	
	for (int j = 0; j < kernel_addr - 2; j++)
		dir_.entries[i++] = (page_table *)0x0;
	
	dir_.entries[i++] = (page_table *)0x00000083;
	dir_.entries[i++] = (page_table *)0x00400083;
	
	for (int j = 0; j < 1024 - kernel_addr - 2; j++)
		dir_.entries[i++] = (page_table *)0x0;
	
	uint32_t addr = (uint32_t)&dir_.entries - 0xC0000000;

	asm volatile("mov %0, %%eax\nmov %%eax, %%cr3" : : "r"(addr) : "%eax");

	printf("paging init\n");

	printf("%02x", *((uint8_t*)0x00000000));

}