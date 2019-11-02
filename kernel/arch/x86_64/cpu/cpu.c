#include "cpu.h"
#include <irq/isr.h>
#include <kmesg.h>
#include <arch/cpu.h>
#include <cpu/cpu_data.h>
#include <cpu/gdt.h>
#include <cpu/smp.h>

#include <symtab/symtab.h>

void cpu_dump_regs_irq(irq_cpu_state_t *state) {
	char buf[256 + 4];

	symtab_format_from_addr(buf, 256, state->rip);

	kmesg("cpu", "rax: %016lx rbx: %016lx", state->rax, state->rbx);
	kmesg("cpu", "rcx: %016lx rdx: %016lx", state->rcx, state->rdx);
	kmesg("cpu", "rsi: %016lx rdi: %016lx", state->rsi, state->rdi);
	kmesg("cpu", "rsp: %016lx rbp: %016lx", state->rsp, state->rbp);
	kmesg("cpu", "rip: %016lx rfl: %016lx", state->rip, state->rflags);
	kmesg("cpu", "rip -> %s", buf);
	kmesg("cpu", "r15: %016lx r14: %016lx", state->r15, state->r14);
	kmesg("cpu", "r13: %016lx r12: %016lx", state->r13, state->r12);
	kmesg("cpu", "r11: %016lx r10: %016lx", state->r11, state->r10);
	kmesg("cpu", "r9:  %016lx r8:  %016lx", state->r9,  state->r8);
	kmesg("cpu", "cs:  %016x ss:  %016x", state->cs,  state->ss);
	
	uintptr_t cr2;
	asm volatile ("mov %%cr2, %0" : "=r"(cr2));

	kmesg("cpu", "cr2: %016lx", cr2);
}

void cpu_trace_stack(void) {
	uintptr_t *bp;
	asm volatile ("mov %%rbp, %0" : "=r"(bp));

	size_t idx = 0;

	for (uintptr_t ip = bp[1]; bp; ip = bp[1], bp = (uintptr_t *)bp[0]) {
		char buf[256 + 4];

		symtab_format_from_addr(buf, 256, ip);

		kmesg("cpu", "#%lu: [%016lx] -> %s", idx++, ip, buf);
	}
}

// only works in the kernel address space
int cpu_get_id(void) {
	if (!gdt_get_gs_base()) {
		return 0; // bsp by default
	}

	int id;
	asm volatile ("mov %%gs:(%1), %0" : "=r"(id) : "r"(offsetof(cpu_data_t, cpu_id)));
	return id;
}

// -----

void arch_cpu_dump_state(void *state) {
	cpu_dump_regs_irq((irq_cpu_state_t *)state);
}

void arch_cpu_trace_stack(void) {
	cpu_trace_stack();
}

void arch_cpu_halt_forever(void) {
	asm volatile (	"cli\n"
					"1:\n"
						"hlt\n"
						"jmp 1b" : : : "memory");
}

int arch_cpu_get_this_id(void) {
	return cpu_get_id();
}

int arch_cpu_get_count(void) {
	// smp only counts APs and not the BSP
	return smp_get_working_count() + 1;
}
