#include "pmm.h"

uint32_t pmm_stack[1048576]={0x0};
page_metadata_t pmm_metadata[1048576]={{0}};

uint32_t pmm_stack_pointer = 0;
uint32_t pmm_stack_size = 0;
uint32_t pmm_stack_max_size = 0;

extern int kprintf(const char*, ...);

inline bool is_available(uint32_t page_begin, multiboot_info_t *mbt) {
	multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)(mbt->mmap_addr + 0xC0000000);
	while((uint32_t)mmap < (mbt->mmap_addr + 0xC0000000 + mbt->mmap_length)) {
		
		uint32_t begin = (uint32_t)mmap->addr;
		uint32_t end = begin + (uint32_t)mmap->len;

		if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE) {
			if (page_begin >= begin && page_begin + 0x1000 <= end) return false;
		}

		mmap = (multiboot_memory_map_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size));
	}

	return true;
}

void pmm_push(uint32_t val) {
	if (pmm_stack_size > 1048576) return;
	if ((val & 0xFFF) != 0) kprintf("Warning! Added an unaligned page to page frame pool! Address added: %08x\n", val);
	pmm_stack[pmm_stack_pointer] = val;
	pmm_stack_pointer++;
	pmm_stack_size++;
}

uint32_t pmm_pop() {
	if(pmm_stack_size == 0) {asm volatile ("cli"); kprintf("out of physical memory!"); while(1);}
	pmm_stack_pointer--;
	pmm_stack_size--;
	return pmm_stack[pmm_stack_pointer];
}

page_metadata_t* get_page_metadata(uint32_t addr) {
	uint32_t d = addr & 0xFFFFF000;
	d >>= 12;
	return &pmm_metadata[d];
}

void set_page_metadata(uint32_t addr, page_metadata_t metadata) {
	uint32_t d = addr & 0xFFFFF000;
	d >>= 12;
	pmm_metadata[d] = metadata;
}

void pmm_init(multiboot_info_t *mbt) {

	uint32_t mem_sz = mbt->mem_upper;

	uint32_t old_sz = mem_sz * 1024;

	mem_sz -= 12288;
	mem_sz *= 1024;

	if (old_sz - mem_sz > old_sz) {
		asm volatile ("cli"); kprintf("insufficient memory, please add at least %u bytes!", (old_sz - mem_sz) - old_sz); while(1);
	}

	for (uint32_t i = 0; i < mem_sz; i += 4096) {
		if (is_available(0xC00000 + i, mbt)) {
			pmm_push(0xC00000 + i);
			pmm_stack_max_size ++;
		} else {
			kprintf("unavailable address %08x", 0xC00000 + i);
		}
	} 

	kprintf("[kernel] pmm ok\n");
}

void *pmm_alloc() {
	uint32_t addr = pmm_pop();

	get_page_metadata(addr)->refcount = 1;

	return (void *)addr;
}

void pmm_free(void *ptr) {
	uint32_t page = (uint32_t)ptr;
	page &= 0xFFFFF000;
	if (page <= 0x800000) return;
	
	if (get_page_metadata(page)->refcount == 1) {
		get_page_metadata(page)->refcount = 0;
		pmm_push(page);
	} else if (get_page_metadata(page)->refcount > 1) {
		get_page_metadata(page)->refcount--;
	}
}

size_t free_pages() {
	return pmm_stack_size;
}

size_t max_pages() {
	return pmm_stack_max_size;
}

size_t used_pages() {
	return pmm_stack_max_size - pmm_stack_size;
}