#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#include <multiboot.h>

typedef struct {

	uint32_t refcount;

} page_metadata_t;

void pmm_init(multiboot_info_t *);
void *pmm_alloc();
void pmm_free(void*);
page_metadata_t* get_page_metadata(uint32_t);
void set_page_metadata(uint32_t, page_metadata_t);
size_t free_pages();
size_t max_pages();
size_t used_pages();

#endif