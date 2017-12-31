#ifndef VSPRINTF_H
#define VSPRINTF_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char*);
int vsprintf(char *, const char *, va_list);
int sprintf(char *, const char *, ...);
int printf(const char*, ...);

#endif