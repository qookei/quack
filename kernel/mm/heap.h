#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void *kmalloc(size_t);
void *krealloc(void *, size_t);
void *kcalloc(size_t, size_t);
void kfree(void *);

#endif
