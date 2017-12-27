#include "heap.h"

memory_chunk *first;
uint32_t mem_top;

void heap_init() {
	first 	= 0;
	mem_top = 0;
	void *page = (void *)pmm_alloc();
	map_page(page, 0x0, 0x3);
	for(int i = 0; i < 4096; i++) ((uint8_t*)(0x0))[i] = 0;
	first->allocated = false;
	first->size = 0x1000;
	first->prev = 0;
	first->next = 0;
}

extern int printf(const char*, ...);

void expand_heap() {
	void *page = (void *)pmm_alloc();
	map_page(page, (void *)(mem_top + 0x1000), 0x3);
	for(int i = 0; i < 4096; i++) ((uint8_t*)(mem_top + 0x1000))[i] = 0;
	mem_top += 0x1000;
}

void *kmalloc(size_t size) {

	//printf("kekle\n");

	memory_chunk *res = 0;
	for (memory_chunk *chunk = first; chunk != 0 && res == 0; chunk = chunk->next) {
		if (chunk->size > size && !chunk->allocated) {
			res = chunk;
			break;
		}
	}
	
	if (res == 0) {

		memory_chunk *m = first;
		while(m->next != 0) m = m->next;
		
		uint32_t expanded = m->size;

		while (expanded < size) {
			expand_heap();
			expanded += 0x1000;
		}

		m->size = expanded;

		//return kmalloc(size);						//restart alloc process with more mem
		res = 0;
		for (memory_chunk *chunk = first; chunk != 0 && res == 0; chunk = chunk->next) {
			if (chunk->size > size && !chunk->allocated) {
				res = chunk;
				break;
			}
		}
	}
	
	if (res->size >= size + sizeof(memory_chunk) + 1) {
		memory_chunk *tmp = (memory_chunk *)((size_t)res + sizeof(memory_chunk) + size);
		
		tmp->allocated = false;
		tmp->prev = res;
		tmp->next = res->next;
		tmp->size = res->size - size - sizeof(memory_chunk);
		
		if (tmp->next != 0)
			tmp->next->prev = tmp;
			
		res->size = size;
		res->next = tmp;	
	}
	
	res->allocated = true;
	
	return (void *)(((size_t)res)+sizeof(memory_chunk));
}


void kfree(void *ptr) {
	memory_chunk *chunk = (memory_chunk *)((size_t)ptr - sizeof(memory_chunk));
	
	chunk->allocated = false;
	if (chunk->prev != 0 && !chunk->prev->allocated) {
		chunk->prev->next = chunk->next;
		chunk->prev->size += chunk->size + sizeof(memory_chunk);
		if (chunk->next != 0) {
			chunk->next->prev = chunk->prev;
		}
		
		chunk = chunk->prev;
	}
	
	if (chunk->next != 0 && !chunk->next->allocated) {
		chunk->size += chunk->next->size + sizeof(memory_chunk);
		chunk->next = chunk->next->next;
		if (chunk->next != 0) {
			chunk->next->prev = chunk;
		}
	}
}