#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>

#include "ctype.h"
#include "string.h"

char* itoa(uint32_t n, char* s, int base);
int atoi(const char *str);
long atol(const char *str);
long long atoll(const char *str);

#endif