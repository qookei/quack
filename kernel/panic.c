#include "panic.h"
#include <interrupt/isr.h>
#include <trace/stacktrace.h>
#include <paging/paging.h>
#include <mesg.h>

void panic(const char *message, interrupt_cpu_state *state, int regs, int pf_error) {
	
	uint32_t cr3 = get_cr3();

	set_cr3(def_cr3());

	#ifdef PANIC_NICE
	early_mesg(LEVEL_WARN, "kernel", "            ..");
	early_mesg(LEVEL_WARN, "kernel", "           ( '`<");
	early_mesg(LEVEL_WARN, "kernel", "            )(");
	early_mesg(LEVEL_WARN, "kernel", "     ( ----'  '.");
	early_mesg(LEVEL_WARN, "kernel", "     (         ;");
	early_mesg(LEVEL_WARN, "kernel", "      (_______,'");
	early_mesg(LEVEL_WARN, "kernel", " ~^~^~^~^~^~^~^~^~^~^~");  
	#endif
	early_mesg(LEVEL_WARN, "kernel", "Kernel panic!");
	early_mesg(LEVEL_WARN, "kernel", "Message: '%s'", message);
	if (regs) {
	
		uint32_t fault_addr;
   		asm volatile("mov %%cr2, %0" : "=r" (fault_addr));
		if (pf_error) {
			early_mesg(LEVEL_WARN, "kernel", "cr2: %08x", fault_addr);
			early_mesg(LEVEL_WARN, "kernel", "cr3: %08x", cr3);
		}
		early_mesg(LEVEL_WARN, "kernel", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x", state->eax, state->ebx, state->ecx, state->edx, state->ebp);
		early_mesg(LEVEL_WARN, "kernel", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x", state->eip, state->eflags, state->esp, state->edi, state->esi);
		early_mesg(LEVEL_WARN, "kernel", "cs: %04x ds: %04x", state->cs, state->ds);
	}
	
	stack_trace(20);
	
	asm volatile ("1:\nhlt\njmp 1b");
}
