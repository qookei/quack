#ifndef PANIC
#define PANIC

#include <interrupt/isr.h>

void panic(const char *, interrupt_cpu_state *, bool, bool);

#endif