#include "spinlock.h"
#include <arch/cpu.h>

void spinlock_lock(spinlock_t *lock) {
	if (lock->locked)
		spinlock_wait(lock);

	lock->locked = 1;
}

int spinlock_try_lock(spinlock_t *lock) {
	if (lock->locked)
		return 0;
	lock->locked = 1;
	return 1;
}

void spinlock_wait(spinlock_t *lock) {
	while(lock->locked) {
		ARCH_CPU_SPIN_PAUSE;
	}
}

void spinlock_release(spinlock_t *lock) {
	lock->locked = 0;
}
