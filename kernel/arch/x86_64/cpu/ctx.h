#ifndef CTX_H
#define CTX_H

typedef struct {
	uint64_t ds, cs;
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8,
			 rdi, rsi, rbp, rdx, rcx, rbx, rax;
	uint64_t rip, rsp, rflags;
} ctx_cpu_state_t;

#endif
