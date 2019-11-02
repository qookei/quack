#include "symtab.h"
#include <mm/heap.h>
#include <vsnprintf.h>
#include <string.h>

struct symbol {
	char *name;
	uintptr_t base;
	size_t len;
};

static struct symbol *syms;
static size_t n_symbols;

void symtab_init(size_t n_syms) {
	syms = kcalloc(n_syms, sizeof(struct symbol));
	n_symbols = n_syms;
}

void symtab_define_symbol(size_t idx, char *name, uintptr_t base, size_t len) {
	if (idx > n_symbols)
		return;

	syms[idx].name = kmalloc(strlen(name) + 1);
	strcpy(syms[idx].name, name);

	syms[idx].base = base;
	syms[idx].len = len;
}

void symtab_format_from_addr(char *dest, size_t dest_size, uintptr_t addr) {
	struct symbol sym = {"", 0, 0};

	for (size_t i = 0; i < n_symbols; i++) {
		if (addr >= syms[i].base && addr <= (syms[i].base + syms[i].len)) {
			sym = syms[i];
			break;
		}
	}

	if (!sym.len)
		snprintf(dest, dest_size, "??+?");
	else
		snprintf(dest, dest_size, "%s+0x%lx", sym.name, addr - sym.base);
}
