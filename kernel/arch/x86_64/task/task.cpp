#include <arch/task.h>
#include <cpu/ctx.h>
#include <cpu/gdt.h>
#include <cpu/cpu.h>
#include <cpu/cpu_data.h>
#include <irq/isr.h>

struct arch_task_impl {
	ctx_cpu_state_t state;
	void *vmm_ctx;
	uint8_t io_bitmap[8192];
};

arch_task::arch_task() :_impl_data{new arch_task_impl} {
	_impl_data->state.ss = 0x1b;
	_impl_data->state.rsp = 0;
	_impl_data->state.rflags = 0x202;
	_impl_data->state.cs = 0x13;
	_impl_data->state.rip = 0;
}

arch_task::~arch_task() {
	delete _impl_data;
}

uintptr_t &arch_task::ip() {
	return _impl_data->state.rip;
}

uintptr_t &arch_task::sp() {
	return _impl_data->state.rsp;
}

void *&arch_task::vmm_ctx() {
	return _impl_data->vmm_ctx;
}

void arch_task::load_irq_state(void *state) {
	irq_cpu_state_t *s = (irq_cpu_state_t *)state;
	_impl_data->state.r15 = s->r15;
	_impl_data->state.r14 = s->r14;
	_impl_data->state.r13 = s->r13;
	_impl_data->state.r12 = s->r12;
	_impl_data->state.r11 = s->r11;
	_impl_data->state.r10 = s->r10;
	_impl_data->state.r9  = s->r9;
	_impl_data->state.r8  = s->r8;
	_impl_data->state.rdi = s->rdi;
	_impl_data->state.rsi = s->rsi;
	_impl_data->state.rbp = s->rbp;
	_impl_data->state.rdx = s->rdx;
	_impl_data->state.rcx = s->rcx;
	_impl_data->state.rbx = s->rbx;
	_impl_data->state.rax = s->rax;
	_impl_data->state.ss  = s->ss;
	_impl_data->state.rsp = s->rsp;
	_impl_data->state.cs  = s->cs;
	_impl_data->state.rip = s->rip;
	_impl_data->state.rflags = s->rflags;
}

[[noreturn]] void arch_task::enter() {
	cpu_data_t *d = cpu_data_get(cpu_get_id());
	tss_update_io_bitmap(d->tss, _impl_data->io_bitmap);
	ctx_switch(&_impl_data->state, (uintptr_t)_impl_data->vmm_ctx);
}

extern "C" void internal_task_idle(uintptr_t);

void arch_task_idle_cpu(void) {
	uintptr_t stack_ptr = cpu_data_get(cpu_get_id())->stack_ptr;

	internal_task_idle(stack_ptr);
}
