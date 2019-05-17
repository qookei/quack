#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint64_t ds;
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8,
			 rdi, rsi, rbp, rdx, rcx, rbx, rax;
	uint64_t int_no, err;
	uint64_t rip, cs, rflags;
	uint64_t rsp, ss;
} irq_cpu_state_t;

typedef int (* irq_handler_t)(irq_cpu_state_t *);

int isr_register_handler(uint8_t irq, irq_handler_t handler);
int isr_unregister_handler(uint8_t irq);

#endif
