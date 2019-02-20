#include "kmesg.h"

void kmesg(const char *src, const char *fmt, ...) {
	char fmt_buf[1024];
	va_list va;
	va_start(va, fmt);
	vsprintf(fmt_buf, fmt, va);
	va_end(va);
	
	char out_buf[1162];
	sprintf(out_buf, "%s: %s\n", src, fmt_buf);
	
	for (size_t i = 0; i < strlen(out_buf); i++)
		debug_write(out_buf[i]);
}
