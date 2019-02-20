#ifndef KMESG_H
#define KMESG_H

#include <stddef.h>
#include <stdint.h>
#include <io/debug_port.h>
#include <vsprintf.h>

void kmesg(const char *src, const char *fmt, ...);

#endif
