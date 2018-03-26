#ifndef ELF
#define ELF

#include <fs/vfs.h>
#include <paging/paging.h>
#include <multiboot.h>

#define ELF_ARCH_32BIT (1)
#define ELF_ARCH_64BIT (2)
#define ELF_BYTEORDER_LENDIAN (1)
#define ELF_386_MACHINE (3)
#define SH_UNDEF (0)
#define SEGTYPE_NONE  (0)
#define SEGTYPE_LOAD  (1)
#define SEGTYPE_DLINK (2)
#define SEGTYPE_INTER (3)
#define SEGTYPE_NOTE  (4)

enum elf_type {ELF_NONE=0, ELF_REL=1, ELF_EXEC=2};

typedef struct {
	uint8_t mag_num[4];
	uint8_t arch;
	uint8_t byte_order;
	uint8_t elf_ver;
	uint8_t os_abi;
	uint8_t abi_ver;
	uint8_t unused[7];
	uint16_t file_type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t hsize;
	uint16_t ph_ent_size;
	uint16_t ph_ent_cnt;
	uint16_t sh_ent_size;
	uint16_t sh_ent_cnt;
	uint16_t sh_name_index;
} __attribute__((packed)) elf_hdr;

typedef struct {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entsize;
} elf_section_header;

typedef struct {
	uint32_t type;
	uint32_t data_offset;
	uint32_t load_to;
	uint32_t undefined;
	uint32_t size_in_file;
	uint32_t size_in_mem;
	uint32_t flags;
	uint32_t align;
} elf_program_header;

typedef struct {

	uint32_t entry_addr;
	uint32_t page_direc;
	bool success_ld;

} elf_loaded;

elf_loaded prepare_elf_for_exec(const char* name);

#endif