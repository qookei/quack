#include "isr.h"
#include <trace/stacktrace.h>
#include <mesg.h>
#include <paging/paging.h>
#include <sched/sched.h>
#include <sched/task.h>
#include <panic.h>

static interrupt_handler_f *interrupt_handlers[IDT_size] = {0};
static pid_t userspace_interrupt_handlers[IDT_size] = {0};

int isr_in_kdir = 0;
uint32_t isr_old_cr3;

volatile int is_servicing_driver = 0;

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

const char* int_names[] = {
		"#DE", "#DB", "-", "#BP", "#OF", "#BR", "#UD", "#NM",
		"#DF", "-", "#TS", "#NP", "#SS", "#GP", "#PF", "-",
		"#MF", "#AC", "#MC", "#XM", "#VE", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "#SX", "-", "-"
};

void dispatch_interrupt(interrupt_cpu_state r) {
	enter_kernel_directory();

	int handled = 0;

	if (interrupt_handlers[r.interrupt_number] != NULL) {
		handled = interrupt_handlers[r.interrupt_number](&r);
	}

	if (r.interrupt_number < 32 && !handled && r.cs != 0x08) {
		sched_kill(sched_get_current()->pid, r.eax, SIGILL);
	
	}

	if (r.interrupt_number >= 32 && userspace_interrupt_handlers[r.interrupt_number]) {
		pid_t p = userspace_interrupt_handlers[r.interrupt_number];

		if (task_wakeup_irq(sched_get_task(p))) {
			sched_move_after(sched_get_current(), sched_get_task(p));
		}

		is_servicing_driver = 1;
	}

	if (r.interrupt_number < 32 && !handled) {
		if (r.interrupt_number == 0x08) {
			early_mesg(LEVEL_ERR, "kernel", "double fault!");
			while(1) asm volatile ("hlt");
		}

			early_mesg(LEVEL_ERR, "interrupt", "Kernel Panic!");
			early_mesg(LEVEL_ERR, "interrupt", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x", r.eax, r.ebx, r.ecx, r.edx, r.ebp);
			early_mesg(LEVEL_ERR, "interrupt", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x", r.eip, r.eflags, r.esp, r.edi, r.esi);
			early_mesg(LEVEL_ERR, "interrupt", "cs: %04x ds: %04x", r.cs, r.ds);
			early_mesg(LEVEL_ERR, "interrupt", "exception:  %s    error code: %08x", int_names[r.interrupt_number], r.err_code);
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

void register_userspace_handler(uint8_t int_no, pid_t pid) {
	if (!userspace_interrupt_handlers[int_no]) {
		userspace_interrupt_handlers[int_no] = pid;
		early_mesg(LEVEL_INFO, "interrupt", "process %d registered a handler for interrupt %u", pid, int_no);
	}
}

void unregister_userspace_handler(uint8_t int_no, pid_t pid) {
	if (userspace_interrupt_handlers[int_no] == pid)
		userspace_interrupt_handlers[int_no] = 0;
}

void unregister_all_handlers_for_pid(pid_t pid) {
	for (size_t i = 0; i < IDT_size; i++) {
		if (userspace_interrupt_handlers[i] == pid)
			userspace_interrupt_handlers[i] = 0;
	}
}

