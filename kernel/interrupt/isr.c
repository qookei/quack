#include "isr.h"
#include <trace/stacktrace.h>
#include <mesg.h>
#include <paging/paging.h>
#include <tasking/tasking.h>
#include <panic.h>


static interrupt_handler_f *interrupt_handlers[IDT_size] = {0};

int isr_in_kdir = 0;
uint32_t isr_old_cr3;

void enter_kernel_directory() {
	isr_old_cr3 = get_cr3();
	set_cr3(def_cr3());
}

void leave_kernel_directory() {
	set_cr3(isr_old_cr3);
}

void pic_eoi(uint32_t r) {
	if (r > 0x1F) {
				
		uint8_t irq = r - 0x20;
		
		if (irq >= 8) {
			// send eoi to slave
			outb(0xA0, 0x20);
		}
		// send eoi to master
		outb(0x20, 0x20);
	}
}

const char* int_names[] = {"Division by zero", "Debug", "NMI", "Breakpoint", "Overflow", "Bound range exceeded",
	   		"Invalid opcode", "Device not available", "Double fault",
			"Coprocessor segment overrun", "Invalid TSS", "Segment not present", "Stack segment fault", "General protection fault", "Page fault",
			"Reserved", "x87 Floating point exception", "Alignment check", "Machine check", "SIMD Floating point exception", "Virtualization exception",
			"Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Security exception",
			"Reserved", "Triple fault"};
	
void dispatch_interrupt(interrupt_cpu_state r) {

	enter_kernel_directory();

	early_mesg(LEVEL_DBG, "interrupt", "servicing interrupt %u(exc: %s)", r.interrupt_number, (r.interrupt_number >= 32) ? "no" : "yes");

	int handled = 0;
		
	if (interrupt_handlers[r.interrupt_number] != NULL) {
		handled = interrupt_handlers[r.interrupt_number](&r);
	}

	if (r.interrupt_number < 32 && !handled && r.cs != 0x08) {
	
		early_mesg(LEVEL_ERR, "fixme", "add task killing code! isr.cc:62");
		// TODO

		tasking_schedule_next();
	    tasking_schedule_after_kill();

	}


	if (r.interrupt_number < 32 && !handled ) {
		
		if (r.interrupt_number == 0x08) {
			early_mesg(LEVEL_ERR, "kernel", "double fault!");
			
			while(1) asm volatile ("hlt");	
		}
		
			early_mesg(LEVEL_ERR, "interrupt", "Kernel Panic!\nUnhandled exception!\n");
			early_mesg(LEVEL_ERR, "interrupt", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x\n", r.eax, r.ebx, r.ecx, r.edx, r.ebp);
			early_mesg(LEVEL_ERR, "interrupt", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x\n", r.eip, r.eflags, r.esp, r.edi, r.esi);
			early_mesg(LEVEL_ERR, "interrupt", "cs: %04x ds: %04x\n", r.cs, r.ds);
			early_mesg(LEVEL_ERR, "interrupt", "exception:  %s\nerror code: %08x\n", int_names[r.interrupt_number], r.err_code);
			stack_trace(20);
			while(1) asm volatile ("hlt");
	}
	
	
	pic_eoi(r.interrupt_number);
	leave_kernel_directory();

}

int register_interrupt_handler(uint8_t int_no, interrupt_handler_f handler) {
	if (interrupt_handlers[int_no] == NULL) {
		interrupt_handlers[int_no] = handler;
		return 1;
	}
	
	return 0;
}

int unregister_interrupt_handler(uint8_t int_no, interrupt_handler_f handler) {
	if (interrupt_handlers[int_no] == handler) {
		interrupt_handlers[int_no] = NULL;
		return 1;
	}
	
	return 0;
}
