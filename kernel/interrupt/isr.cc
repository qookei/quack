#include "isr.h"
#include "../trace/stacktrace.h"

#define isr_count 10

static interrupt_handler_f *interrupt_handlers[IDT_size][isr_count] = {{0}};

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

extern int printf(const char *, ...);

extern "C" {

void dispatch_interrupt(interrupt_cpu_state r) {
	bool handled = false;
	
	for (size_t i = 0; i < isr_count; ++i) {
		if (interrupt_handlers[r.interrupt_number][i] != NULL) {
			handled = interrupt_handlers[r.interrupt_number][i](&r);
			if (handled) break;
		}
	}
	
	const char* int_names[] = {"Division by zero", "Debug", "NMI", "Breakpoint", "Overflow", "Bound range exceeded", "Invalid opcode", "Device not available", "Double fault",
					"Coprocessor segment overrun", "Invalid TSS", "Segment not present", "Stack segment fault", "General protection fault", "Page fault",
					"Reserved", "x87 Floating point exception", "Alignment check", "Machine check", "SIMD Floating point exception", "Virtualization exception",
					"Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Security exception",
					"Reserved", "Triple fault"};
	
	if (r.interrupt_number < 32 && !handled) {
		
		if (r.interrupt_number == 0x08) {
			// double fault
			// were doomed
	
			
			printf("Double fault!\n");
			
			while(1) asm volatile ("hlt");
	
		}
		
		printf("Unhandled exception!\n");
		printf("eax: 0x%x ebx: 0x%x ecx: 0x%x edx: 0x%x ebp: 0x%x esi: 0x%x edi: 0x%x\n", r.eax, r.ebx, r.ecx, r.edx, r.ebp, r.esi, r.edi);
		printf("int_no: 0x%x(%s) err_code: 0x%x eip: 0x%x cs: 0x%x eflags: 0x%x esp: 0x%x ss: 0x%x\n", r.interrupt_number, int_names[r.interrupt_number], r.err_code, r.eip, r.cs, r.eflags, r.esp, r.ss);
		stack_trace(20);
		while(1) asm volatile ("hlt");
	}
	
	
	printf("int %x\n", r.interrupt_number);

	pic_eoi(r.interrupt_number);
}

}

bool register_interrupt_handler(uint8_t int_no, interrupt_handler_f handler) {
	bool registered = false;
	for (size_t i = 0; i < isr_count; ++i) {
		if (interrupt_handlers[int_no][i] == NULL) {
			interrupt_handlers[int_no][i] = handler;
			registered = true;
		}
	}
	
	return registered;
}

bool unregister_interrupt_handler(uint8_t int_no, interrupt_handler_f handler) {
	bool unregistered = false;
	for (size_t i = 0; i < isr_count; ++i) {
		if (interrupt_handlers[int_no][i] == handler) {
			interrupt_handlers[int_no][i] = NULL;
			unregistered = true;
		}
	}
	
	return unregistered;
}