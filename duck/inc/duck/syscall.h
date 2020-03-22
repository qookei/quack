#ifndef DUCK_SYSCALL_H
#define DUCK_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include <duck/types.h>

duck_error_t duck_syscall_2(int syscall_no, uintptr_t a0, uintptr_t a1);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //DUCK_SYSCALL_H
