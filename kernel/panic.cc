#include "panic.h"
#include <interrupt/isr.h>
#include <trace/stacktrace.h>
#include <paging/paging.h>

extern int printf(const char *, ...);

void panic(const char *message, interrupt_cpu_state *state, bool regs, bool pf_error) {
	
	set_cr3(def_cr3());

	// TODO: use a logging system and not the standard output

	printf("\e[1m");
	printf("\e[47m");
	printf("\e[31m");
	printf("Kernel Panic!\n");
	printf("%s\n", message);

	if (regs) {
	
		uint32_t fault_addr;
   		asm volatile("mov %%cr2, %0" : "=r" (fault_addr));

		printf("cr2: %08x\n", fault_addr);
		printf("eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x\n", state->eax, state->ebx, state->ecx, state->edx, state->ebp);
		printf("eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x\n", state->eip, state->eflags, state->esp, state->edi, state->esi);
		printf("cs: %04x ds: %04x\n", state->cs, state->ds);
	}

	stack_trace(20, 0);
	asm volatile ("1:\nhlt\njmp 1b");
}