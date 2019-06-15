#ifndef CMDLINE_H
#define CMDLINE_H

#include <stddef.h>

void cmdline_init(char *);

int cmdline_is_enabled(const char *);
size_t cmdline_get_value_count(const char *);
char **cmdline_get_values(const char *);
int cmdline_has_value(const char *, const char *);

#endif
