#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

struct spinlock {
	spinlock() = default;
	~spinlock() = default;
	spinlock(const spinlock &) = delete;
	spinlock &operator=(const spinlock &) = delete;

	void lock();
	bool try_lock();
	void unlock();
private:
	std::atomic_bool _locked;
};

#endif
