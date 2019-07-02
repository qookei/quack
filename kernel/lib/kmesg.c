#include "kmesg.h"
#include <vsnprintf.h>
#include <arch/io.h>

void kmesg(const char *src, const char *fmt, ...) {
	char fmt_buf[1024 + 16];
	va_list va;
	va_start(va, fmt);
	vsnprintf(fmt_buf, 1024, fmt, va);
	va_end(va);

	char out_buf[1162 + 16];
	snprintf(out_buf, 1162, "%s: %s\n", src, fmt_buf);

	char *out = out_buf;
	while(*out)
		arch_debug_write(*out++);
}