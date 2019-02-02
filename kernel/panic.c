#include "panic.h"
#include <interrupt/isr.h>
#include <trace/stacktrace.h>
#include <paging/paging.h>
#include <mesg.h>

void panic(const char *message, interrupt_cpu_state *state, int regs, int pf_error) {
	
	uint32_t cr3 = get_cr3();

	set_cr3(def_cr3());

	early_mesg(LEVEL_ERR, "kernel", "Kernel panic!");
	early_mesg(LEVEL_ERR, "kernel", "Message: '%s'", message);
	if (regs) {
	
		uint32_t fault_addr;
   		asm volatile("mov %%cr2, %0" : "=r" (fault_addr));
		if (pf_error) {
			early_mesg(LEVEL_ERR, "kernel", "cr2: %08x", fault_addr);
			early_mesg(LEVEL_ERR, "kernel", "cr3: %08x", cr3);
		}
		early_mesg(LEVEL_ERR, "kernel", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x", state->eax, state->ebx, state->ecx, state->edx, state->ebp);
		early_mesg(LEVEL_ERR, "kernel", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x", state->eip, state->eflags, state->esp, state->edi, state->esi);
		early_mesg(LEVEL_ERR, "kernel", "cs: %04x ds: %04x", state->cs, state->ds);
	}
	
	stack_trace(20);

	early_panic(LEVEL_ERR, "kernel", "halting");

	asm volatile ("1:\nhlt\njmp 1b");
}
