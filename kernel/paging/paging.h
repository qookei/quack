#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include "pmm.h"

struct page_directory {
	uint32_t *entries[1024] __attribute__((aligned(4096)));
};

void paging_init(void);
uint32_t alloc_clean_page();
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);

#endif