#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <stdint.h>
#include <stddef.h>
#include <kmesg.h>

void stack_trace(uintptr_t);

#endif
