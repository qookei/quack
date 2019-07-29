#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_DEBUG_LOG 1

uintptr_t syscall_invoke(uintptr_t *arg0, uintptr_t *arg1,
		uintptr_t *arg2, uintptr_t *arg3,
		uintptr_t *arg4, uintptr_t *arg5,
		void *irq_state);

#endif
