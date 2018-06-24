#include "heap.h"
#include <string.h>
#include <trace/stacktrace.h>

#define ALIGNED(x) ((uint32_t)(x) + align_pointer((uint32_t)(x)))

extern int kprintf(const char *, ...);
extern int printf(const char *, ...);

block_t *root = NULL;
void *heap_begin;
void *heap_end;

void block_dump(block_t *blk) {
	kprintf("block @ 0x%08p\n", blk);
	kprintf("prev -> 0x%08p\n", blk->prev);
	kprintf("next -> 0x%08p\n", blk->next);
	kprintf("used -> %s\n",     blk->used ? "true" : "false");
	kprintf("magic-> %08x\n",   blk->magic);
	kprintf("magic2->%08x%08x\n",   (uint32_t)(blk->magic2 >> 32), (uint32_t)blk->magic2);
	kprintf("magic3->%08x%08x\n",   (uint32_t)(blk->magic3 >> 32), (uint32_t)blk->magic3);
	if(blk->magic != MAGIC_FREED && blk->magic != MAGIC_FRESH)
		kprintf("block has invalid magic!\n");
	
	if (blk->magic2 != MAGIC_STAT1 && blk->magic3 != MAGIC_STAT2)
		kprintf("more block magic is invalid!\n");
}

void heap_block_scan(uint32_t begin, block_t *highlight) {
	uint32_t found_blocks = 0;
	kprintf("   block    prev     next     size     used\n");
	for (uint32_t i = begin; i < (uint32_t)heap_end - sizeof(block_t); i += 4) {
		block_t *bl = (block_t *)i;
		if (bl->magic2 == MAGIC_STAT1 && bl->magic3 == MAGIC_STAT2) {
			kprintf(" %c %08p %08p %08p %08x %s\n", highlight == bl ? '*' : ' ', bl, bl->prev, bl->next, bl->size, bl->used ? "true" : "false");
			found_blocks++;
		}
	}
	
	kprintf("heap: found %u blocks after scanning\n", found_blocks);
}

void inline heap_add_page() {
	void *page = pmm_alloc();		// alloc physical page
	map_page(page, heap_end, 0x3);	// map it at end of heap
	heap_end = (void *)((uint32_t)heap_end + 0x1000);	// increase end of heap
}

uint32_t inline align_pointer(uint32_t ptr) {
	uint32_t ptr2 = ptr + sizeof(block_t);
	uint32_t add_to = 0x10 - (ptr2 & 0xF);
	return add_to;
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
		root->magic2 = MAGIC_STAT1;
		root->magic3 = MAGIC_STAT2;
		root->used = false;
	}

	block_t *bl = root;
	while (bl->next) {
		if (bl->size >= size && !bl->used)
			return; // there's a large enough block
		if (bl == bl->next) {
			heap_block_scan((uint32_t)bl, NULL);
			printf("heap: loopback block found, halting!\n");
			while(1) 
				asm volatile("hlt");
		}
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
			block_t *new_block = (block_t *)ALIGNED((uint32_t)bl + bl->size);
			new_block->size = size;
			new_block->used = false;
			new_block->magic = MAGIC_FRESH;
			new_block->magic2 = MAGIC_STAT1;
			new_block->magic3 = MAGIC_STAT2;
			new_block->next = NULL;
			new_block->prev = bl;
			bl->next = new_block;
		} else {
			// need to create a new block on newly expanded memory
			size_t new_size = size + sizeof(block_t) + 0x10;
			size_t pages = new_size / 0x1000 + (new_size & 0xFFF ? 1 : 0);

			block_t *new_block = (block_t *)ALIGNED(heap_end);

			for (size_t i = 0; i < pages; i++)
				heap_add_page();

			new_block->size = size;
			new_block->used = false;
			new_block->magic = MAGIC_FRESH;
			new_block->magic2 = MAGIC_STAT1;
			new_block->magic3 = MAGIC_STAT2;
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
		bl->magic2 = MAGIC_STAT1;
		bl->magic3 = MAGIC_STAT2;

		return (void *)((uint32_t)bl + sizeof(block_t));
	} else if (bl->size > size) {
		// split it the current one into two smaller one
		if (bl->size - size > sizeof(block_t) + 0x10) {
			// we can fit another block
			size_t smaller_size = bl->size - size - sizeof(block_t);

			block_t *new_block = (block_t *)ALIGNED(((uint32_t)bl + size + sizeof(block_t)));
			
			new_block->next = bl->next;
			bl->next = new_block;
			new_block->prev = bl;

			if (new_block->next && new_block->next->prev)
				new_block->next->prev = new_block;

			new_block->size = smaller_size;
			new_block->magic = MAGIC_FRESH;
			new_block->magic2 = MAGIC_STAT1;
			new_block->magic3 = MAGIC_STAT2;
			new_block->used = false;
			bl->size = size;
			bl->used = true;
			bl->magic = MAGIC_FRESH;
			bl->magic2 = MAGIC_STAT1;
			bl->magic3 = MAGIC_STAT2;
			
			return (void *)((uint32_t)bl + sizeof(block_t));
		} else {
			// dont split since it would waste memory or is impossible

			bl->used = true;
			bl->magic = MAGIC_FRESH;
			bl->magic2 = MAGIC_STAT1;
			bl->magic3 = MAGIC_STAT2;
			
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
		kprintf("heap: possible double free on %08x\n", (uint32_t)pointer);
		stack_trace(20, 0);
	
		block_dump(block);

		return;
	}
	
	if (block->magic != MAGIC_FRESH || block->magic2 != MAGIC_STAT1 || block->magic3 != MAGIC_STAT2) {
		kprintf("heap: possible corruption/unaligned pointer because invalid magic\n");
		stack_trace(20, 0);
	
		block_dump(block);
	
		return;
	}
	

	block->used = false;
	block->magic = MAGIC_FREED;
	
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
