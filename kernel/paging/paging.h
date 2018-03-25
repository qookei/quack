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

void crosspd_memcpy(uint32_t dst_pd, void *dst_addr, uint32_t src_pd, void *src_addr, size_t sz);
void crosspd_memset(uint32_t dst_pd, void *dst_addr, int num, size_t sz);
void alloc_mem_at(uint32_t pd, uint32_t where, size_t pages, uint32_t flags);

uint32_t create_page_directory(multiboot_info_t*);
void destroy_page_directory(void*);

#endif