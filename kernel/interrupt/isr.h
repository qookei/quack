#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <io/ports.h>

#include "idt.h"

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, edx, ecx,
              ebx, eax;
    uint32_t interrupt_number, err_code;
    uint32_t eip, cs, eflags, esp, ss;
} interrupt_cpu_state;

typedef bool (interrupt_handler_f)(interrupt_cpu_state *);

void enter_kernel_directory();
void leave_kernel_directory();

bool register_interrupt_handler(uint8_t int_no, interrupt_handler_f handler);
bool unregister_interrupt_handler(uint8_t int_no, interrupt_handler_f handler);

#endif