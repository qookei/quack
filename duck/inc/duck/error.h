#ifndef DUCK_ERROR_H
#define DUCK_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

enum {
	duck_error_none,
	duck_error_fault,
	duck_error_out_of_range,
	duck_error_invalid_call,
};

//! Convert an error code to a string
static inline __attribute__((always_inline)) const char *duck_strerror(int c) {
	switch (c) {
		case duck_error_none: return "success";
		case duck_error_fault: return "vm fault";
		case duck_error_out_of_range: return "out of range";
		case duck_error_invalid_call: return "invalid call";
		default: return "invalid error";
	}
}

#ifndef DUCK_IN_KERNEL

#define ___DUCK_STR__(x) #x
#define ___DUCK_STR(x) ___DUCK_STR__(x)

static inline __attribute__((always_inline)) size_t ___duck_strlen(const char *str) {
	size_t n = 0;
	while (str[n++])
		;
	return n;
}

//! Evaluate the given expression and check for errors
#define DUCK_CHECK(expr) \
	do { \
		int ret = (expr); \
		if (ret != duck_error_none) {\
			const char *msg = "Error check failed for " #expr ", that is in " __FILE__ ":" ___DUCK_STR(__LINE__) " Returned error:"; \
			const char *err = duck_strerror(ret); \
			duck_debug_log(msg, ___duck_strlen(msg)); \
			duck_debug_log(err, ___duck_strlen(err)); \
			__builtin_trap(); \
		}\
	} while(0);

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif //DUCK_ERROR_H
