#include "paging.h"
#include <tasking/tasking.h>
#include <interrupt/isr.h>
#include <trace/stacktrace.h>
#include <panic.h>
#include <mesg.h>

struct page_directory dir_;

uint32_t current_pd;

void set_cr3(uint32_t addr) {
	current_pd = addr;
	asm volatile("mov %0, %%eax\nmov %%eax, %%cr3" : : "r"(addr) : "%eax");
}

uint32_t get_cr3() {
	uint32_t a;
	asm volatile("mov %%cr3, %0" : "=r"(a) : : );
	return a;
}

uint32_t def_cr3() {
	return (uint32_t)dir_.entries - 0xC0000000;
}

static inline void tlb_flush_entry(uint32_t addr) {
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}


uint32_t pt_f[1024] __attribute__((aligned(4096))) = {0};

// 0xFFC00000
#define PT 0xFFC00000

void *get_phys_at_next(uint32_t pd, void *addr) {
	return get_phys(pd, (void *)(((uint32_t)addr + 0x1000) & 0xFFFFF000));
}

void crosspd_memcpy(uint32_t dst_pd, void *dst_addr, uint32_t src_pd, void *src_addr, size_t sz) {
	uint32_t src_phys;
	uint32_t dst_phys;
	
	uint32_t src_off;
	uint32_t dst_off;

	src_phys = (uint32_t)get_phys(src_pd, src_addr);
	dst_phys = (uint32_t)get_phys(dst_pd, dst_addr);

	src_off = (uint32_t)src_addr & 0xFFF;
	src_phys = src_phys & 0xFFFFF000; 
	
	dst_off = (uint32_t)dst_addr & 0xFFF;
	dst_phys = dst_phys & 0xFFFFF000;

	while (sz > 0) {
		size_t copy_size = sz > 0x1000 ? 0x1000 : sz;
		// map source
		map_page((void *)src_phys, (void *)0xEF000000, 0x3);

		src_addr = (void *)((uint32_t)src_addr + 0x1000);
		src_phys = (uint32_t)get_phys(src_pd, src_addr) & 0xFFFFF000;

		if (src_off + copy_size >= 0x1000 && src_phys)
			map_page((void *)src_phys, (void *)0xEF001000, 0x3);

		// map destination
		map_page((void *)dst_phys, (void *)0xEF002000, 0x3);

		dst_addr = (void *)((uint32_t)dst_addr + 0x1000);		
		dst_phys = (uint32_t)get_phys(dst_pd, dst_addr) & 0xFFFFF000;

		if (dst_off + copy_size >= + 0x1000 && dst_phys)
			map_page((void *)dst_phys, (void *)0xEF003000, 0x3);

		// copy
		memcpy((void *)(0xEF002000 + dst_off), (void *)(0xEF000000 + src_off), copy_size);

		// unmap source
		unmap_page((void *)0xEF000000);
		if (src_off + copy_size >= 0x1000 && src_phys)
			unmap_page((void *)0xEF001000);

		// unmap destination
		unmap_page((void *)0xEF002000);
		if (dst_off + copy_size >= 0x1000 && dst_phys)
			unmap_page((void *)0xEF003000);


		sz -= copy_size;
		
	}

}

void crosspd_memset(uint32_t dst_pd, void *dst_addr, int num, size_t sz) {
	uint32_t dst_phys;
	uint32_t dst_off;

	dst_phys = (uint32_t)get_phys(dst_pd, dst_addr);

	dst_off = dst_phys & 0xFFF;
	dst_phys &= ~0xFFF;

	uint32_t set = 0;

	while (1) {
		size_t set_size = sz > 0x1000 ? 0x1000 : sz;
		
		map_page((void *)dst_phys, (void *)0xE0002000, 0x3);
		if (dst_phys + dst_off + set_size > dst_phys + 0x1000)
			map_page((void *)(dst_phys + 0x1000), (void *)0xE0003000, 0x3);

		memset((void *)(0xE0002000 + dst_off), num, set_size);

		unmap_page((void *)0xE0002000);
		if (dst_phys + dst_off + set_size > dst_phys + 0x1000)
			unmap_page((void *)0xE0003000);

		set += set_size;
		if (set >= sz) break;

	}

}



void *alloc_mem_at(uint32_t pd, uint32_t where, size_t pages, uint32_t flags) {
	if (where + pages * 0x1000 > 0xC0000000) return NULL;
	uint32_t opd = get_cr3();
	void *dst_phys;
	set_cr3(pd);
	for (size_t i = 0; i < pages; i++) {
		void *p = pmm_alloc();
		map_page(p, (void *)(where + i * 0x1000), flags);
		if (!i) dst_phys = p;
	}
	set_cr3(opd);
	return dst_phys;
}

uint32_t alloc_clean_page() {

	uint32_t *pd = (uint32_t *)0xFFFFF000;

	for(uint32_t i = 0; i < 1024; i++) pt_f[i] = 0;

	pt_f[0] = (uint32_t)pmm_alloc() | 0x3;
	uint32_t r = pd[1023];
	pt_f[1023] = r;
	pd[1023] = (((uint32_t)pt_f - 0xC0000000) | 0x3);
	tlb_flush_entry(PT);

	uint8_t *pt = (uint8_t *)PT;
	
	for(uint32_t i = 0; i < 4096; i++) pt[i] = 0;

	tlb_flush_entry(PT);
	pd[1023] = r;
	tlb_flush_entry(PT);

	return pt_f[0] & 0xFFFFF000;
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
	if ((((uint32_t)physaddr) & 0xFFF) != 0 || (((uint32_t)virtualaddr)&0xFFF) != 0) {
		early_mesg(LEVEL_WARN, "vmm", "map_page with unaligned address(es)!");
		return;
	}

	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	uint32_t *pd = (uint32_t *)0xFFFFF000;
	if (!(pd[pdindex] & 0x1)) pd[pdindex] = alloc_clean_page() | (flags & 0xFFF);
	tlb_flush_entry(0xFFC00000);

	uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * pdindex);

	if(pt[ptindex] & 0x1) {
 		early_mesg(LEVEL_WARN, "vmm", "map_page on already mapped address! %08p %08p %08p", physaddr, virtualaddr, __builtin_return_address(0));
		return;
	}
	
	pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | 0x01; // Present

	tlb_flush_entry((uint32_t)virtualaddr);
}

void unmap_page(void *virtualaddr) {
	if ((((uint32_t)virtualaddr)&0xFFF) != 0) {
		early_mesg(LEVEL_WARN, "vmm", "unmap_page with unaligned address!");
		return;
	}
 
	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	uint32_t *pd = (uint32_t *)0xFFFFF000;
	if (!(pd[pdindex] & 0x1)) {
		early_mesg(LEVEL_WARN, "vmm", "unmap_page on nonexistent page directory entry!");
	}

	uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * pdindex);
	pt[ptindex] = 0x00000000;

	tlb_flush_entry((uint32_t)virtualaddr);

}

void *get_phys(uint32_t pd, void *virtualaddr) {
	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	map_page((void *)pd, (void *)0xE0000000, 0x3);
	uint32_t pt = ((uint32_t *)0xE0000000)[pdindex];
	unmap_page((void *)0xE0000000);
	if (!(pt & 0xFFF)) {
		return NULL;
	}

	map_page((void *)(pt & 0xFFFFF000), (void *)0xE0000000, 0x3);
	uint32_t page = ((uint32_t *)0xE0000000)[ptindex];
	unmap_page((void *)0xE0000000);
	if (!(page & 0xFFF)) {
		return NULL;
	}
	
	return (void*)((page & 0xFFFFF000) + ((uint32_t)virtualaddr & 0xFFF));
}

uint32_t get_flag(uint32_t pd, void *virtualaddr) {
	uint32_t pdindex = (uint32_t)virtualaddr >> 22;
	uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

	map_page((void *)pd, (void *)0xE0000000, 0x3);
	uint32_t pt = ((uint32_t *)0xE0000000)[pdindex];
	unmap_page((void *)0xE0000000);
	if (!(pt & 0xFFF)) {
		return 0;
	}

	map_page((void *)(pt & 0xFFFFF000), (void *)0xE0000000, 0x3);
	uint32_t page = ((uint32_t *)0xE0000000)[ptindex];
	unmap_page((void *)0xE0000000);

	return page & 0xFFF;
}


uint32_t create_page_directory() {
	uint32_t kernel_addr = (0xC0000000 >> 22);

	uint32_t tmp_addr = (uint32_t)pmm_alloc();
	map_page((void *)tmp_addr, (void*)0xEDA9B000, 0x3);

	uint32_t addr = 0xEDA9B000;
	uint32_t *new_dir = (uint32_t *)addr;

	memset(new_dir, 0, 0x1000);

	new_dir[kernel_addr] = 0x00000083;
	new_dir[kernel_addr + 1] = 0x00400083;
	new_dir[kernel_addr + 2] = 0x00800083;
	
	new_dir[1023] = (tmp_addr | 0x3);

	unmap_page((void*)0xEDA9B000);

	return tmp_addr;

}

void destroy_page_directory(void *pd) {
	if (get_cr3() == (uint32_t)pd) {
		early_mesg(LEVEL_WARN, "vmm", "destroy_page_directory on used page directory!");
		return;
	}

	pmm_free(pd);
}

extern task_t* current_task;

extern uint32_t isr_old_cr3;
extern int isr_in_kdir;

extern void mem_dump(void*,size_t,size_t);

int page_fault(interrupt_cpu_state *state) {

	uint32_t fault_addr;
   	asm volatile("mov %%cr2, %0" : "=r" (fault_addr));

   	uint32_t fault_cr3;
   	asm volatile("mov %%cr3, %0" : "=r" (fault_cr3));

	// The error code gives us details of what happened.
	int present   = !(state->err_code & 0x1); 	// Page not present
	int rw = state->err_code & 0x2;				// Write operation?
	int us = state->err_code & 0x4;				// Processor was in user-mode?
	int reserved = state->err_code & 0x8;		// Overwritten CPU-reserved bits of page entry?
	int id = state->err_code & 0x10;			// Caused by an instruction fetch?


	if (us) {

		if (present) early_mesg(LEVEL_WARN, "vmm", "present ");
		if (rw)	early_mesg(LEVEL_WARN, "vmm", "write ");
		if (reserved) early_mesg(LEVEL_WARN, "vmm", "rb overwritten ");
		if (id) early_mesg(LEVEL_WARN, "vmm", "instr fetch ");	
		early_mesg(LEVEL_WARN, "vmm", "at %08x faulting process regs at crash:", fault_addr);
		early_mesg(LEVEL_WARN, "vmm", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x", state->eax, state->ebx, state->ecx, state->edx, state->ebp);
		early_mesg(LEVEL_WARN, "vmm", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x", state->eip, state->eflags, state->esp, state->edi, state->esi);
		early_mesg(LEVEL_WARN, "vmm", "cs: %04x ds: %04x", state->cs, state->ds);
		early_mesg(LEVEL_WARN, "vmm", "code:");

		set_cr3(def_cr3());

		task_t *fault_proc = current_task;
		tasking_schedule_next();
		kill_task_raw(fault_proc);
		tasking_schedule_next();

		isr_old_cr3 = current_task->cr3;
		isr_in_kdir = 0;


		tasking_schedule_after_kill();
	}

	panic("Page fault", state, 1, 1);
	return 1;
}

void paging_init(void) {

	register_interrupt_handler(14, page_fault);

	uint32_t kernel_addr = (0xC0000000 >> 22);

	uint32_t i = 0;

	uint32_t addr = (uint32_t)&dir_.entries - 0xC0000000;

	dir_.entries[i++] = (uint32_t *)0x0;
	dir_.entries[i++] = (uint32_t *)0x0;
	dir_.entries[i++] = (uint32_t *)0x0;
	
	for (uint32_t j = 0; j < kernel_addr - 3; j++)
		dir_.entries[i++] = (uint32_t *)0x0;
	
	dir_.entries[i++] = (uint32_t *)0x00000083;
	dir_.entries[i++] = (uint32_t *)0x00400083;
	dir_.entries[i++] = (uint32_t *)0x00800083;
	
	for (uint32_t j = 0; j < 1024 - kernel_addr - 4; j++)
		dir_.entries[i++] = (uint32_t *)0x0;

	dir_.entries[i++] = (uint32_t *)(addr | 0x3);

	set_cr3(addr);


	early_mesg(LEVEL_INFO, "vmm", "paging ok");

}
