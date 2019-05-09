#include "panic.h"
//#include <interrupt/isr.h>
#include <trace/stacktrace.h>
//#include <paging/paging.h>
#include <kmesg.h>

void panic(const char *message, void *state, int regs, int pf_error) {

	//uint32_t cr3 = get_cr3();

	//set_cr3(def_cr3());

	kmesg("kernel", "Kernel panic!");
	kmesg("kernel", "Message: '%s'", message);
	if (regs) {
		uint32_t fault_addr;
		asm volatile("mov %%cr2, %0" : "=r" (fault_addr));
		if (pf_error) {
			kmesg("kernel", "cr2: %08x", fault_addr);
			//kmesg("kernel", "cr3: %08x", cr3);
		}
		//kmesg("kernel", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x", state->eax, state->ebx, state->ecx, state->edx, state->ebp);
		//kmesg("kernel", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x", state->eip, state->eflags, state->esp, state->edi, state->esi);
		//kmesg("kernel", "cs: %04x ds: %04x", state->cs, state->ds);
	}

	stack_trace(20);

	kmesg("kernel", "halting");

	asm volatile ("1:\nhlt\njmp 1b");
}
