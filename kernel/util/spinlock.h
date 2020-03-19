#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

struct spinlock {
	friend void swap(spinlock &a, spinlock &b) {
		a._locked = b._locked.exchange(a._locked);
	}

	spinlock() = default;
	~spinlock() = default;
	spinlock(const spinlock &) = delete;
	spinlock &operator=(const spinlock &) = delete;

	spinlock(spinlock &&other) {
		swap(*this, other);
	}

	spinlock &operator=(spinlock &&other) {
		swap(*this, other);
		return *this;
	}

	void lock();
	bool try_lock();
	void unlock();
private:
	std::atomic_bool _locked;
};

#endif
