#ifndef PMM_H
#define PMM_H

#include <multiboot.h>
#include <stdint.h>
#include <stddef.h>

void pmm_init(multiboot_memory_map_t *mmap, size_t mmap_len);
void *pmm_alloc(size_t count);
void pmm_free(void *mem, size_t count);

#endif
