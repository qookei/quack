#ifndef UTIL_H
#define UTIL_H

#define _STR(x) #x
#define STR(x) _STR(x)

#define assert(x) {if(!(x)) __assert_failure(STR(x), __FILE__, __LINE__, __func__);}

[[noreturn]] void __assert_failure(const char *, const char *, int, const char *); 

#endif
