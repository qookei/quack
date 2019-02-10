#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef int32_t pid_t;

#include <io/ports.h>

#include "idt.h"

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, edx, ecx,
              ebx, eax;
    uint32_t interrupt_number, err_code;
    uint32_t eip, cs, eflags, esp, ss;
} interrupt_cpu_state;

typedef int (interrupt_handler_f)(interrupt_cpu_state *);

void enter_kernel_directory();
void leave_kernel_directory();

int register_interrupt_handler(uint8_t int_no, interrupt_handler_f handler);
int unregister_interrupt_handler(uint8_t int_no, interrupt_handler_f handler);

void register_userspace_handler(uint8_t int_no, pid_t pid);
void unregister_userspace_handler(uint8_t int_no, pid_t pid);
void unregister_all_handlers_for_pid(pid_t pid);

#endif
