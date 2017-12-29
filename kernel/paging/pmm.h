#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#include "../multiboot.h"

void pmm_init(multiboot_info_t *);
void *pmm_alloc();
void pmm_free(void*);
size_t free_pages();
size_t max_pages();

#endif