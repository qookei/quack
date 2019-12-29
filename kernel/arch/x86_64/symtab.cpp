#include <symtab/symtab.h>
#include <multiboot.h>
#include <mm/mm.h>
#include <kmesg.h>

#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define STT_FUNC 2

typedef struct {
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
	uint32_t st_name;
	unsigned char st_info;
	unsigned char st_other;
	uint16_t st_shndx;
	uint64_t st_value;
	uint64_t st_size;
} Elf64_Sym;

void arch_symtab_parse(multiboot_info_t *mboot) {
	if (mboot->flags & MULTIBOOT_INFO_ELF_SHDR) {
		Elf64_Shdr *shdrs = (Elf64_Shdr *)(mboot->u.elf_sec.addr + VIRT_PHYS_BASE);

		size_t symtab = 0xFFFFFFFFFFFFFFFF, strtab = 0xFFFFFFFFFFFFFFFF;
		for (size_t i = 0; i < mboot->u.elf_sec.num; i++) {
			if (symtab == 0xFFFFFFFFFFFFFFFF
					&& shdrs[i].sh_type == SHT_SYMTAB)
				symtab = i;

			if (strtab == 0xFFFFFFFFFFFFFFFF
					&& shdrs[i].sh_type == SHT_STRTAB)
				strtab = i;

			if (strtab != 0xFFFFFFFFFFFFFFFF
					&& symtab != 0xFFFFFFFFFFFFFFFF)
				break;
		}

		if (symtab == 0xFFFFFFFFFFFFFFFF || strtab == 0xFFFFFFFFFFFFFFFF) {
			kmesg("symtab", "failed to find %s", 
					symtab == 0xFFFFFFFFFFFFFFFF ? "symtab" 
					: "strtab");
			return;
		}

		size_t n_syms = shdrs[symtab].sh_size / shdrs[symtab].sh_entsize;

		symtab_init(n_syms);

		for (size_t i = 0; i < n_syms; i++) {
			Elf64_Sym sym = ((Elf64_Sym *)(shdrs[symtab].sh_addr + VIRT_PHYS_BASE))[i];
			char *name = (char *)(shdrs[strtab].sh_addr + VIRT_PHYS_BASE + sym.st_name);

			if ((sym.st_info & 0xf) != STT_FUNC)
				continue;

			symtab_define_symbol(i, name, sym.st_value, sym.st_size);
		}
	}
}
