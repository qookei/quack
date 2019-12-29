#ifndef ARCH_IO_H
#define ARCH_IO_H

#include <stddef.h>

void arch_debug_write(char);
extern "C" __attribute__((weak))
void arch_mem_fast_memcpy(void *dst, void *src, size_t count);
extern "C" __attribute__((weak))
void arch_mem_fast_memset(void *dst, int val, size_t count);


#endif
