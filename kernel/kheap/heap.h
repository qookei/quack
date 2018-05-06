#ifndef HEAP_H
#define HEAP_H

#include <paging/paging.h>

#define MAGIC_FRESH 0xCAFEBABE
#define MAGIC_FREED 0x7BADCAFE

typedef struct block {
	size_t size;
	bool used;
	struct block *next;
	struct block *prev;

	uint32_t magic;
} block_t;

void *kmalloc(size_t) __attribute__((malloc));
void kfree(void *);
void *krealloc(void *, size_t);

#endif
