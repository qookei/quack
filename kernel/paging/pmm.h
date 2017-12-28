#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

void pmm_init(uint32_t);
void *pmm_alloc();
void pmm_free(void*);
size_t free_pages();
size_t max_pages();

#endif