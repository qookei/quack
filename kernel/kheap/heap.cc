#include "heap.h"

memory_chunk *first;
uint32_t mem_top;
extern int kprintf(const char*, ...);
extern void* memcpy(void*, const void*, size_t);

void heap_init() {
	first 	= (memory_chunk *)0x1000;
	mem_top = 0x1000;
	void *page = (void *)pmm_alloc();
	map_page(page, first, 0x3);
	first->allocated = false;
	first->size = 0x1000 - sizeof(memory_chunk);
	first->prev = 0;
	first->next = 0;
	kprintf("[kernel] heap ok\n");
}


void expand_heap() {
	mem_top += 0x1000;
	void *page = (void *)pmm_alloc();
	map_page(page, (void *)(mem_top), 0x3);
}

void *kmalloc(size_t size) {
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
		
		uint32_t expanded = 0;

		for (int i = 0; i < size + sizeof(memory_chunk); i += 0x1000) {
			expand_heap();
			expanded += 0x1000;
		}

		memory_chunk *tmp = (memory_chunk *)((size_t)m + sizeof(memory_chunk) + m->size);
		
		tmp->allocated = false;
		tmp->prev = m;
		tmp->next = 0;
		tmp->size = expanded - sizeof(memory_chunk);
		tmp->prev->next = tmp;

		res = 0;
		for (memory_chunk *chunk = first; chunk != 0 && res == 0; chunk = chunk->next) {
			if (chunk->size > size && !chunk->allocated) {
				res = chunk;
				break;
			}
		}
		kprintf("%08x\n", (uint32_t)res);
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

void *krealloc(void *ptr, size_t size) {
	uint32_t addr = (uint32_t)ptr;
	memory_chunk *chunk = (memory_chunk *)((size_t)addr - sizeof(memory_chunk));
	uint32_t old_size = chunk->size;
	
	if (old_size >= size)
		return ptr;
	
	void *_new = kmalloc(size);
	memcpy(_new, ptr, old_size);
	
	kfree(ptr);
	
	return _new;
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