#ifndef UUID_H
#define UUID_H

#include <stdint.h>
#include <stddef.h>

void uuid_generate(uint64_t seed, uint8_t *dest);
void uuid_to_string(char *dest, uint8_t *uuid);

#endif
