#ifndef VSPRINTF_H
#define VSPRINTF_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

void vsnprintf(char *, size_t, const char *, va_list);
void snprintf(char *, size_t, const char *, ...);

#endif
