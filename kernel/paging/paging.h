#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include "pmm.h"

struct page_directory {
	uint32_t *entries[1024] __attribute__((aligned(4096)));
};

void set_cr3(uint32_t);
uint32_t get_cr3();
uint32_t def_cr3();

void paging_init(void);
uint32_t alloc_clean_page();
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);
void* get_phys(void*);

uint32_t create_page_directory(multiboot_info_t*);
void destroy_page_directory(void*);

#endif