#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>
#include "../paging/paging.h"

struct memory_chunk {

	struct memory_chunk* next;
	struct memory_chunk* prev;
	
	bool allocated;
	size_t size;

};

typedef struct memory_chunk memory_chunk;

void heap_init();
void *kmalloc(size_t);
void *krealloc(void *,size_t);
void kfree(void *);

#endif