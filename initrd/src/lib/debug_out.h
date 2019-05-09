#ifndef DEBUG_OUT_H
#define DEBUG_OUT_H

#include "ctype.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void uvsprintf(char *buf, const char *fmt, va_list arg);
void uprintf(void (*write)(char *), const char *fmt, ...);
void usprintf(char *buf, const char *fmt, ...);

void debugf(const char *fmt, ...);
void debug_hex_dump(uint8_t *data, size_t length);

#endif
