#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include "pmm.h"

struct page_table {
	uint32_t entries[1024] __attribute__((aligned(4096)));	
};

struct page_dir_internal {
	page_table entries[1024] __attribute__((aligned(4096)));
};

struct page_directory {
	page_table *entries[1024] __attribute__((aligned(4096)));
};

void paging_init(void);


#endif