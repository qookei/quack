#ifndef SPINLOCK_H
#define SPINLOCK_H

typedef struct {
	volatile int locked;
} spinlock_t;

void spinlock_lock(spinlock_t *);
void spinlock_release(spinlock_t *);

#endif
