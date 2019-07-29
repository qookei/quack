#include "syscall.h"

#include <arch/io.h>

uintptr_t syscall_invoke(uintptr_t *arg0, uintptr_t *arg1,
		uintptr_t *arg2, uintptr_t *arg3,
		uintptr_t *arg4, uintptr_t *arg5,
		void *irq_state) {

	if(*arg0 == SYS_DEBUG_LOG) {
		arch_debug_write(*arg1);
	}

	return 0;
}
