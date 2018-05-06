#include "heap.h"
#include <string.h>

extern int kprintf(const char *, ...);

block_t *root = NULL;
void *heap_begin;
void *heap_end;

void inline heap_add_page() {
	void *page = pmm_alloc();		// alloc physical page
	map_page(page, heap_end, 0x3);	// map it at end of heap
	heap_end = (void *)((uint32_t)heap_end + 0x1000);	// increase end of heap
}

void heap_expand_end(size_t size) {
	if (!heap_begin) {
		// heap is uninitialized
		heap_begin = (void *)0x1000;
		heap_end = heap_begin;
		heap_add_page();
		root = (block_t *)heap_begin;
		root->size = 0x1000 - sizeof(block_t);
		root->next = root->prev = NULL;
		root->magic = MAGIC_FRESH;
		root->used = false;
	}

	block_t *bl = root;
	while (bl->next) {
		if (bl->size >= size && !bl->used)
			return; // there's a large enough block
		bl = bl->next;
	}
	// bl points to last block
	if (!bl->used) {
		if (bl->size >= size) { // nothing to do
			bl->magic = MAGIC_FRESH; // so free wont think this is double free
			return;
		}

		// we need to expand
		size_t pages = size / 0x1000 + (size & 0xFFF ? 1 : 0);
		for (size_t i = 0; i < pages; i++)
			heap_add_page();
		// expand bl size by amount of added pages * page size
		bl->size += pages * 0x1000;

	} else {
		// we need a new block
		if ((uint32_t)bl + bl->size + sizeof(block_t) <= (uint32_t)heap_end) {
			// we have free space in this block
			size_t left_over = (uint32_t)heap_end - (uint32_t)bl -
									bl->size - sizeof(block_t);
			size_t pages = left_over / 0x1000 + (left_over & 0xFFF ? 1 : 0);
			for (size_t i = 0; i < pages; i++)
				heap_add_page();

			// create new block
			block_t *new_block = (block_t *)((uint32_t)bl + bl->size);
			new_block->size = size;
			new_block->used = false;
			new_block->magic = MAGIC_FRESH;
			new_block->next = NULL;
			new_block->prev = bl;
			bl->next = new_block;
		} else {
			// need to create a new block on newly expanded memory
			size_t new_size = size + sizeof(block_t);
			size_t pages = new_size / 0x1000 + (new_size & 0xFFF ? 1 : 0);

			block_t *new_block = (block_t *)heap_end;

			for (size_t i = 0; i < pages; i++)
				heap_add_page();

			new_block->size = size;
			new_block->used = false;
			new_block->magic = MAGIC_FRESH;
			new_block->next = NULL;
			new_block->prev = bl;
			bl->next = new_block;
		}
	}
}


void *kmalloc(size_t size) {
	heap_expand_end(size); // will not do anything when initializing

	// find large enough page

	block_t *bl = root;
	while (bl->next) {
		if (bl->size >= size && !bl->used) break;
		bl = bl->next;
	}

	// found block

	if (bl->size == size) {
		// mark it as used and return since it's exactly the size

		bl->used = true;
		bl->magic = MAGIC_FRESH;

		return (void *)((uint32_t)bl + sizeof(block_t));
	} else if (bl->size > size) {
		// split it the current one into two smaller one
		if (bl->size - size > sizeof(block_t)) {
			// we can fit another block
			size_t smaller_size = bl->size - size - sizeof(block_t);

			block_t *new_block = (block_t *)((uint32_t)bl + size + sizeof(block_t));
			new_block->next = bl->next;
			bl->next = new_block;
			new_block->prev = bl;

			if (new_block->next && new_block->next->prev)
				new_block->next->prev = new_block;

			new_block->size = smaller_size;
			new_block->magic = MAGIC_FRESH;
			new_block->used = false;
			bl->size = size;
			bl->used = true;

			return (void *)((uint32_t)bl + sizeof(block_t));
		} else {
			// dont split since it would waste memory or is impossible

			bl->used = true;
			bl->magic = MAGIC_FRESH;
			return (void *)((uint32_t)bl + sizeof(block_t));
		}
	} else {
		kprintf("heap: heap_expand_end failed to allocate a large enough block\n");
		return NULL;
	}

}

void kfree(void *pointer) {
	block_t *block = (block_t *)((uint32_t)pointer - sizeof(block_t));

	if (block->magic == MAGIC_FREED || !block->used) {
		kprintf("heap: possible double free on %08x\n",
						(uint32_t)pointer);
		return;
	}

	block->used = false;
	block->magic = MAGIC_FREED;
	
	//kprintf("heap: freeing block of size %u bytes at %08p\n", block->size, pointer);

	/*
	if (block->prev) {
		if (!block->prev->used) {
			// merge back
			block_t *p = block->prev;
			if (p->next)
				p->next = p->next->next;
			if (p->prev)
				p->prev = p->prev->prev;
			p->size += block->size + sizeof(block_t);
			block = p;
		}
	}

	if (block->next) {
		if (!block->next->used) {
			block_t *p = block->next;
			if (p->prev)
				p->prev = p->prev->next;
			if (p->next)
				p->next = p->next->prev;
			p->size += block->size + sizeof(block_t);
			block = p;
		}
	}*/
}

void *krealloc(void *pointer, size_t size) {
	if (!pointer)		// allocate new memory
		return kmalloc(size);

	block_t *block = (block_t *)((uint32_t)pointer - sizeof(block_t));

	if (block->size >= size)		// requested size was smaller, ignore
		return pointer;

	// resize it
	void *ptr = kmalloc(size);
	memcpy(ptr, pointer, block->size);
	kfree(pointer);

	return ptr;
}
