#ifndef DUCK_CALLS_H
#define DUCK_CALLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include <duck/error.h>
#include <duck/types.h>

//! Set the debug log identity (the name displayed before the colon)
duck_error_t duck_set_identity(const char *ident, size_t length);

//! Display a log message
duck_error_t duck_debug_log(const char *message, size_t length);

enum {
	duck_call_set_identity = 1,
	duck_call_debug_log
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif //DUCK_CALLS_H
