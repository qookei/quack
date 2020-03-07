#include "spinlock.h"

void spinlock::lock() {
	while (_locked.load(std::memory_order_acquire))
		;

	_locked.store(true, std::memory_order_acquire);
}

bool spinlock::try_lock() {
	if (_locked.load(std::memory_order_acquire))
		return false;

	_locked.store(true, std::memory_order_acquire);
	return true;
}

void spinlock::unlock() {
	_locked.store(false, std::memory_order_release);
}
