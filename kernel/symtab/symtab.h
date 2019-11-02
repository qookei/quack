#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdint.h>
#include <stddef.h>

void symtab_init(size_t n_syms);
void symtab_define_symbol(size_t idx, char *name, uintptr_t base, size_t len);

void symtab_format_from_addr(char *dest, size_t dest_size, uintptr_t addr);

#endif
