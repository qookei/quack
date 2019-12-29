#include "spinlock.h"
#include <arch/cpu.h>

void spinlock_lock(spinlock_t *lock) {
	arch_cpu_atomic_loop_test_and_set(&lock->locked);
}

void spinlock_release(spinlock_t *lock) {
	arch_cpu_atomic_unset(&lock->locked);
}
