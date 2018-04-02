#include "isr.h"
#include <trace/stacktrace.h>

#include <paging/paging.h>
#include <tasking/tasking.h>
#include <panic.h>

#define isr_count 10

extern int printf(const char *, ...);
extern int kprintf(const char *, ...);

static interrupt_handler_f *interrupt_handlers[IDT_size][isr_count] = {{0}};

bool isr_in_kdir = false;
uint32_t isr_old_cr3;

void enter_kernel_directory() {
	isr_old_cr3 = get_cr3();
	set_cr3(def_cr3());	
}

extern task_t *current_task;

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

#define getreg(x,y) asm volatile ("mov %%" x ", %0" : "=r"(y) : :);

uint32_t ___faulting_cr3;

extern "C" {

// extern void tasking_enter(task_regs_t*, uint32_t);



void dispatch_interrupt(interrupt_cpu_state r) {
	
	getreg("cr3",___faulting_cr3);

	enter_kernel_directory();

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
	

	if (r.interrupt_number < 32 && !handled && current_task->st.cs != 0x08) {
	    set_cr3(def_cr3());

		uint32_t fault_pid = current_task->pid;
		tasking_schedule_next();
		kill_task(fault_pid);

	    printf("Process %u crashed with SIGILL!\n", fault_pid);

	    asm volatile ("mov %0, %%eax; mov %1, %%ebx; jmp tasking_enter" : : "r"(current_task->cr3), "r"(&current_task->st) : "%eax", "%ebx");

	}


	if (r.interrupt_number < 32 && !handled /*|| (tasks[current_task]->regs.cs != 0x08 && r.interrupt_number == 14)*/) {
		
		if (r.interrupt_number == 0x08) {
			printf("Double fault!\n");
			
			while(1) asm volatile ("hlt");	
		}
		
		// if (tasks[current_task]->regs.cs != 0x08) {
		// 	// tasking_kill(current_task);
		// 	// current_task = 0;
		// 	// tasking_schedule();
		// 	// printf("Process terminated\n");
		// 	// leave_kernel_directory();
		// 	// tasking_enter(&tasks[current_task]->regs, tasks[current_task]->page_directory);
			
		// } else {
			printf("\e[H");
			printf("\e[1m");
			printf("\e[47m");
			printf("\e[31m");
			printf("Kernel Panic!\nUnhandled exception!\n");
			printf("eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x\n", r.eax, r.ebx, r.ecx, r.edx, r.ebp);
			printf("eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x\n", r.eip, r.eflags, r.esp, r.edi, r.esi);
			printf("cs: %04x ds: %04x\n", r.cs, r.ds);
			printf("exception:  %s\nerror code: %08x\n", int_names[r.interrupt_number], r.err_code);
			stack_trace(20, 0);
			while(1) asm volatile ("hlt");

		// panic("Unhandled exception!", &r, true, false);

		// }
	}
	
	
	pic_eoi(r.interrupt_number);
	leave_kernel_directory();

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