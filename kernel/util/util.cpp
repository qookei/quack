#include "util.h"

#include <kmesg.h>
#include <panic.h>
#include <stddef.h>

[[noreturn]] void __assert_failure(const char *expr, const char *file, int line, const char *function) {
	kmesg("kernel", "%s:%d: %s: assertion '%s' failed", file, line, function, expr);
	panic(NULL, "assertion failure");
	__builtin_unreachable();
}
