#include "isr.h"
#include <trace/stacktrace.h>
#include <kmesg.h>
//#include <paging/paging.h>
#include <mm/vmm.h>
//#include <sched/sched.h>
//#include <sched/task.h>
#include <panic.h>

static interrupt_handler_f *interrupt_handlers[IDT_size] = {0};
static pid_t userspace_interrupt_handlers[IDT_size] = {0};
static int pre_register_interrupt_count[IDT_size] = {0};

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

void *global_data_ptr = NULL;

#define GLOBAL_DATA_ADDR 0xD0000000

void pic_eoi(uint32_t r) {
	if (r >= 0x20 && r < 0x30) {
		uint8_t irq = r - 0x20;

		if (irq >= 8) {
			// send eoi to slave
			outb(0xA0, 0x20);
		}
		// send eoi to master
		outb(0x20, 0x20);
	}
}

struct global_data {
	uint64_t timer_ticks;
};

void global_data_setup() {
	if (!global_data_ptr) {
		global_data_ptr = pmm_alloc();
		map_page(global_data_ptr, (void *)GLOBAL_DATA_ADDR, 0x3);
		memset((void *)GLOBAL_DATA_ADDR, 0, 0x1000);
	}
}

const char* int_names[] = {
		"#DE", "#DB", "-", "#BP", "#OF", "#BR", "#UD", "#NM",
		"#DF", "-", "#TS", "#NP", "#SS", "#GP", "#PF", "-",
		"#MF", "#AC", "#MC", "#XM", "#VE", "-", "-", "-",
		"-", "-", "-", "-", "-", "-", "#SX", "-", "-"
};

static inline void rdtsc(uint32_t *lo, uint32_t *hi) {
	asm volatile("xor %%eax, %%eax; cpuid; rdtsc" : "=a"(*lo), "=d"(*hi) : : "%ebx", "%ecx");
}

void dispatch_interrupt(interrupt_cpu_state r) {
	enter_kernel_directory();

	if (r.interrupt_number == 32) {
		if (global_data_ptr) {
			struct global_data *d = (struct global_data *)GLOBAL_DATA_ADDR;
			d->timer_ticks++;
		}
	}

	int handled = 0;
/*
	if (interrupt_handlers[r.interrupt_number] != NULL) {
		handled = interrupt_handlers[r.interrupt_number](&r);
	}

	if (r.interrupt_number < 32 && !handled && r.cs != 0x08) {
		kmesg("isr", "process %d caused an exception %s and will be terminated.", sched_get_current()->pid, int_names[r.interrupt_number]);
		sched_kill(sched_get_current()->pid, r.eax, SIGILL);
	}

	if (r.interrupt_number >= 32 && userspace_interrupt_handlers[r.interrupt_number]) {
		pid_t p = userspace_interrupt_handlers[r.interrupt_number];

		if (task_wakeup_irq(sched_get_task(p))) {
			task_t *prev = sched_get_current();

			if (prev->pid != p) {
				task_save_cpu_state(&r, prev);
				sched_move_after(prev, sched_get_task(p));

				task_t *to_run = sched_schedule_next();
				is_servicing_driver = 1;
				pic_eoi(r.interrupt_number); // hackish at best
				task_switch_to(to_run);
			}
		}

		is_servicing_driver = 1;
	} else if (!userspace_interrupt_handlers[r.interrupt_number]) {
		pre_register_interrupt_count[r.interrupt_number]++;
	}

*/
	if (r.interrupt_number < 32 && !handled) {
		if (r.interrupt_number == 0x08) {
			kmesg("kernel", "double fault!");
			while(1) asm volatile ("hlt");
		}

			kmesg("isr", "Kernel Panic!");
			kmesg("isr", "eax: %08x ebx:    %08x ecx: %08x edx: %08x ebp: %08x", r.eax, r.ebx, r.ecx, r.edx, r.ebp);
			kmesg("isr", "eip: %08x eflags: %08x esp: %08x edi: %08x esi: %08x", r.eip, r.eflags, r.esp, r.edi, r.esi);
			kmesg("isr", "cs: %04x ds: %04x", r.cs, r.ds);
			kmesg("isr", "exception:  %s    error code: %08x", int_names[r.interrupt_number], r.err_code);
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
/*
void register_userspace_handler(uint8_t int_no, pid_t pid) {
	if (!userspace_interrupt_handlers[int_no]) {
		userspace_interrupt_handlers[int_no] = pid;
		sched_get_task(pid)->pending_irqs = pre_register_interrupt_count[int_no];
		pre_register_interrupt_count[int_no] = 0;
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
*/
