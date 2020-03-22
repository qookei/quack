#ifndef DUCK_TYPES_H
#define DUCK_TYPES_H

//! Error code
typedef int duck_error_t;

//! Handle that describes an object, relative to a group
typedef uint64_t duck_handle_t;

//! Group ID that holds objects
typedef uint64_t duck_group_t;

//! Thread ID
typedef uint64_t duck_thread_t;

//! Invalid ID value for all types, usually indicates error
#define DUCK_INVALID_ID 0xFFFFFFFFFFFFFFFFull

#endif //DUCK_TYPES_H
