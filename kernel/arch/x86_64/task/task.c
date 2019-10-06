#include <arch/task.h>
#include <cpu/ctx.h> // ctx_cpu_state_t, ctx_switch
#include <cpu/gdt.h> // tss_update_io_bitmap
#include <cpu/cpu.h> // cpu_get_id
#include <cpu/cpu_data.h> // cpu_data_get
#include <irq/isr.h> // irq_cpu_state_t
#include <mm/vmm.h> // pt_t, vmm_...
#include <mm/pmm.h> // pmm_alloc, pmm_free
#include <mm/mm.h> // PAGE_SIZE
#include <mm/heap.h> // kmalloc, kfree
#include <arch/mm.h> // arch_mm_get_kernel_ctx
#include <util.h> // assert
#include <string.h> // memset
#include <kmesg.h> // kmesg

struct arch_task_ {
	ctx_cpu_state_t state;
	pt_t *vmm_ctx;
	uint8_t io_bitmap[8192];
};

arch_task_t *arch_task_create_new(void *mem_ctx) {
	arch_task_t *task = kmalloc(sizeof(arch_task_t));
	memset(task, 0, sizeof(arch_task_t));
	memset(task->io_bitmap, 0x00, 8192); // no port access

	task->state.ss = 0x1b;
	task->state.rsp = 0; // loaded by arch_task_load_stack_ptr
	task->state.rflags = 0x202; // interrupts enabled
	task->state.cs = 0x13;
	task->state.rip = 0; // loaded by arch_task_load_entry_point

	task->vmm_ctx = mem_ctx ? mem_ctx : vmm_new_address_space();

	return task;
}

int arch_task_destroy(arch_task_t *task) {
	// TODO
	return 0; // fail
}

#define LOG_TASK_SWITCHES

__attribute__((noreturn))
void arch_task_switch_to(arch_task_t *task) {
	cpu_data_t *d = cpu_data_get(cpu_get_id());
	tss_update_io_bitmap(d->tss, task->io_bitmap);

#if LOG_TASK_SWITCHED
	kmesg("task", "cpu %d entering task %016p", d->cpu_id, task);
#endif

	ctx_switch(&task->state, (uintptr_t)task->vmm_ctx);
}

int arch_task_save_irq_state(arch_task_t *task, void *irq_state) {
	irq_cpu_state_t *s = irq_state;
	task->state.r15 = s->r15;
	task->state.r14 = s->r14;
	task->state.r13 = s->r13;
	task->state.r12 = s->r12;
	task->state.r11 = s->r11;
	task->state.r10 = s->r10;
	task->state.r9  = s->r9;
	task->state.r8  = s->r8;
	task->state.rdi = s->rdi;
	task->state.rsi = s->rsi;
	task->state.rbp = s->rbp;
	task->state.rdx = s->rdx;
	task->state.rcx = s->rcx;
	task->state.rbx = s->rbx;
	task->state.rax = s->rax;
	task->state.ss  = s->ss;
	task->state.rsp = s->rsp;
	task->state.cs  = s->cs;
	task->state.rip = s->rip;
	task->state.rflags = s->rflags;

	return 1;
}

int arch_task_load_stack_ptr(arch_task_t *task, uintptr_t stack_ptr) {
	task->state.rsp = stack_ptr;
	return 1;
}

int arch_task_load_entry_point(arch_task_t *task, uintptr_t entry) {
	task->state.rip = entry;
	return 1;
}

int arch_task_allow_port_access(arch_task_t *task, uintptr_t port, size_t size) {
	for (size_t i = 0; i < size; i++) {
		uint16_t idx = port + i;
		uint8_t bitmask = ~(1 << (idx % 8));
		uint16_t off = idx / 8;
		task->io_bitmap[off] &= bitmask;
	}

	// TODO: if currently running, reload bitmap, or leave it up to caller?
	return 1;
}

int arch_task_disallow_port_access(arch_task_t *task, uintptr_t port, size_t size) {
	for (size_t i = 0; i < size; i++) {
		uint16_t idx = port + i;
		uint8_t bitmask = (1 << (idx % 8));
		uint16_t off = idx / 8;
		task->io_bitmap[off] |= bitmask;
	}

	// TODO: if currently running, reload bitmap, or leave it up to caller?
	return 1;
}

uintptr_t arch_task_allow_mem_access(arch_task_t *task, struct mem_region *region) {
	// TODO
	return 0;
}

int arch_task_disallow_mem_access(arch_task_t *task, struct mem_region *region) {
	// TODO
	return 0;
}

void *arch_task_get_mem_context(arch_task_t *task) {
	return task->vmm_ctx;
}

int arch_task_alloc_mem_region(arch_task_t *task, struct mem_region *region, int flags) {
	for (uintptr_t ptr = region->start; ptr < region->end; ptr += PAGE_SIZE) {
		void *phys = pmm_alloc(1);
		if (!phys) {
			kmesg("task", "failed to allocate region for task %016p", task);
			return 0;
		}
		vmm_map_pages(task->vmm_ctx, (void *)ptr, phys, 1, 
				vmm_arch_to_vmm_flags(flags));
	}

	return 1;
}

int arch_task_copy_to_mem_region(arch_task_t *task, struct mem_region *region, void *src, size_t count) {
	assert(count <= (region->end - region->start));
	vmm_ctx_memcpy(task->vmm_ctx, (void *)region->start, arch_mm_get_ctx_kernel(-1), src, count);
	return 1;
}

int arch_task_free_mem_region(arch_task_t *task, struct mem_region *region) {
	for (uintptr_t ptr = region->start; ptr < region->end; ptr += PAGE_SIZE) {
		void *virt = (void *)ptr;
		void *phys = (void *)(vmm_get_entry(task->vmm_ctx, virt) & VMM_ADDR_MASK);
		vmm_unmap_pages(task->vmm_ctx, virt, 1);
		pmm_free(phys, 1);
	}
	return 1;
}

void arch_task_idle_cpu(void) {
	uintptr_t stack_ptr = cpu_data_get(cpu_get_id())->stack_ptr;

	internal_task_idle(stack_ptr);
}
