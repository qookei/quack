#include "mesg.h"

void early_mesg(uint8_t level, const char *src, const char *fmt, ...) {
	
	if (level == LEVEL_ERR) {
		#ifndef EARLY_LOG_ERR
		return;
		#endif
	}
	
	if (level == LEVEL_WARN) {
		#ifndef EARLY_LOG_WARN
		return;
		#endif
	}
	
	if (level == LEVEL_INFO) {
		#ifndef EARLY_LOG_INFO
		return;
		#endif
	}
	
	if (level == LEVEL_DBG) {
		#ifndef EARLY_LOG_DBG
		return;
		#endif
	}
	
	char buf[1024];
	va_list va;
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);
	
	char buf2[1152];
	sprintf(buf2, "%s: %s\n", src, buf);
	
	for (size_t i = 0; i < strlen(buf2); i++)
		serial_write_byte(buf2[i]);

}

void early_newl(uint8_t level) {
	
	if (level == LEVEL_ERR) {
		#ifndef EARLY_LOG_ERR
		return;
		#endif
	}
	
	if (level == LEVEL_WARN) {
		#ifndef EARLY_LOG_WARN
		return;
		#endif
	}
	
	if (level == LEVEL_INFO) {
		#ifndef EARLY_LOG_INFO
		return;
		#endif
	}
	
	if (level == LEVEL_DBG) {
		#ifndef EARLY_LOG_DBG
		return;
		#endif
	}
	
	serial_write_byte('\n');

}
