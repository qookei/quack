#include "pmm.h"

uint32_t pmm_stack[1048576]={0x0};

uint32_t pmm_stack_pointer = 0;
uint32_t pmm_stack_size = 0;

extern int printf(const char*, ...);

void pmm_push(uint32_t val) {
	if (pmm_stack_size > 1048576) return;
	pmm_stack[pmm_stack_pointer] = val;
	pmm_stack_pointer++;
	pmm_stack_size++;
}

uint32_t pmm_pop() {
	if(pmm_stack_size == 0) {asm volatile ("cli"); printf("out of physical memory!"); while(1);}
	pmm_stack_pointer--;
	pmm_stack_size--;
	return pmm_stack[pmm_stack_pointer];
}

void pmm_init(uint32_t mem_sz) {
	mem_sz -= 8192;
	mem_sz *= 1024;

	for (uint32_t i = 0; i < mem_sz; i += 4096) {
		pmm_push(0x800000 + i);
	}
}

void *pmm_alloc() {
	// last_free_page
	return (void *)pmm_pop();
}

void pmm_free(void *ptr) {
	uint32_t page = (uint32_t)ptr;
	page &= 0xFFFFF000;
	if (page <= 0x800000) return;
	//printf("0x%x\n", page);

	pmm_push(page);
}