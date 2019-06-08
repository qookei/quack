#include "cpu.h"

#include <irq/isr.h>

#include <kmesg.h>

void cpu_dump_regs_irq(irq_cpu_state_t *state) {
	kmesg("cpu", "rax: %016lx rbx: %016lx", state->rax, state->rbx);
	kmesg("cpu", "rcx: %016lx rdx: %016lx", state->rcx, state->rdx);
	kmesg("cpu", "rsi: %016lx rdi: %016lx", state->rsi, state->rdi);
	kmesg("cpu", "rsp: %016lx rbp: %016lx", state->rsp, state->rbp);
	kmesg("cpu", "rip: %016lx rfl: %016lx", state->rip, state->rflags);
	kmesg("cpu", "r15: %016lx r14: %016lx", state->r15, state->r14);
	kmesg("cpu", "r13: %016lx r12: %016lx", state->r13, state->r12);
	kmesg("cpu", "r11: %016lx r10: %016lx", state->r11, state->r10);
	kmesg("cpu", "r9:  %016lx r8:  %016lx", state->r9,  state->r8);
}
