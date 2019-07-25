#ifndef CTX_H
#define CTX_H

typedef struct {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8,
			 rdi, rsi, rbp, rdx, rcx, rbx, rax;
	uint64_t ss, rsp, rflags, cs, rip;
} ctx_cpu_state_t;

__attribute__((noreturn)) extern void ctx_switch(ctx_cpu_state_t *state, uintptr_t cr3);

#include <arch/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <kmesg.h>
#include <panic.h>

static void test_foo_bar(void) {
	ctx_cpu_state_t state;
	memset (&state, 0, sizeof(state));
	state.ss = 0x1b;
	state.rsp = 0; // no stack
	state.rflags = 0x202;
	state.cs = 0x13;
	state.rip = 0x3000;

	void *mem = pmm_alloc(1);
	pt_t *user = vmm_new_address_space();

	kmesg("ctx", "new user pml4: %016p", user);

	if (!vmm_map_pages(user, (void *)0x3000, mem, 1, VMM_FLAG_USER | VMM_FLAG_WRITE))
		panic(NULL, "vmm_map_pages failed");

	memset((void *)((uintptr_t)mem + VIRT_PHYS_BASE), 0x90, 0x20);
	// NOP x 20
	((uint8_t *)((uintptr_t)mem + VIRT_PHYS_BASE))[0x20] = 0xF4;
	// HLT

	kmesg("ctx", "about to 'jmp userspace'");

	ctx_switch(&state, (uintptr_t)user);
}

#endif
