#ifndef CTX_H
#define CTX_H

#include <stdint.h>

typedef struct {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8,
			 rdi, rsi, rbp, rdx, rcx, rbx, rax;
	uint64_t ss, rsp, rflags, cs, rip;
} ctx_cpu_state_t;

extern "C" __attribute__((noreturn)) void ctx_switch(ctx_cpu_state_t *state, uintptr_t cr3);

#endif
