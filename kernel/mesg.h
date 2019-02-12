#ifndef MESG
#define MESG

#include <stddef.h>
#include <stdint.h>
#include <io/debug_port.h>
#include <vsprintf.h>

#define LEVEL_ERR  1
#define LEVEL_WARN 2
#define LEVEL_INFO 3
#define LEVEL_DBG  4

#ifdef EARLY_LOG_DEFAULT
#define EARLY_LOG_ERR
#undef  EARLY_LOG_WARN
#define EARLY_LOG_INFO
#undef  EARLY_LOG_DBG
#endif

void early_mesg(uint8_t level, const char *src, const char *fmt, ...);
void early_newl(uint8_t level);

#endif
