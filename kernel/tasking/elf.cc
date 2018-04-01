#include "elf.h"

extern int printf(const char*, ...);

unsigned int get_file_size(const char *name) {
	
	struct stat s;
	int r = stat(name, &s);
	if (r != 0) {
		printf("elf: Failed to stat '%s'\n", name);
		return 0;
	}

	return s.st_size;
}

bool elf_check_header(elf_hdr* hdr) {

	if(hdr->mag_num[0] != 0x7f || hdr->mag_num[1] != 'E' || hdr->mag_num[2] != 'L' || hdr->mag_num[3] != 'F') {
		return false;
	}
	if(hdr->arch != ELF_ARCH_32BIT) {
		return false;
	}
	if(hdr->byte_order != ELF_BYTEORDER_LENDIAN) {
		return false;
	}
	if(hdr->elf_ver != 1){
		return false;
	}
	if(hdr->file_type != ELF_REL && hdr->file_type != ELF_EXEC) {
		return false;
	}
	if(hdr->machine != ELF_386_MACHINE) {
		return false;
	}
	return true;
}

void *elf_open(const char* name) {
	int r = open(name, O_RDONLY);

	if(r < 0) {
		printf("elf: Failed to open '%s'\n", name);
		return NULL;
	}
	
	size_t filesize = get_file_size(name);

	void* addr = kmalloc(filesize + 1);
	read(r, (char*)addr, filesize);
	close(r);

	if(!elf_check_header((elf_hdr *)addr)){
		printf("elf: Failed to verify header for '%s'\n", name);
		return NULL;
	}
	
	return addr;
}

elf_section_header *elf_get_section_header(void *elf_file, int num) {
	elf_hdr *hdr = (elf_hdr *)elf_file;
	return (elf_section_header *)((uint8_t*)(elf_file) + hdr->shoff + hdr->sh_ent_size * num);
}

elf_program_header *elf_get_program_header(void *elf_file, int num) {
	elf_hdr *hdr = (elf_hdr *)elf_file;
	return (elf_program_header *)((uint8_t*)(elf_file)+hdr->phoff+hdr->ph_ent_size*num);
}

const char *elf_get_section_name(void *elf_file, int num) {
	elf_hdr *hdr = (elf_hdr *)elf_file;
	return (hdr->sh_name_index == SH_UNDEF) ? "no section" : (const char*)elf_file + elf_get_section_header(elf_file, hdr->sh_name_index)->offset + elf_get_section_header(elf_file, num)->name;
}

extern multiboot_info_t *mbootinfo;

elf_loaded prepare_elf_for_exec(const char* name) {
    elf_loaded result;
    result.success_ld = false;
    result.page_direc = 0;
    result.entry_addr = 0;

    void *elf_file = elf_open(name);

	if(elf_file == NULL)
		return result;

	elf_hdr *hdr = (elf_hdr *)elf_file;

	uint32_t pagedir = create_page_directory(mbootinfo);

	for(int i=0; i < hdr->ph_ent_cnt; i++) {
		elf_program_header *phdr = elf_get_program_header(elf_file, i);

		if(phdr->type != SEGTYPE_LOAD) {
			continue;
		}

		uint32_t sz = phdr->size_in_mem / 0x1000;
		if (phdr->size_in_mem & 0xFFF) sz++;

		alloc_mem_at(pagedir, phdr->load_to & 0xFFFFF000, sz, 0x7);
		crosspd_memset(pagedir, (void*)phdr->load_to, 0, phdr->size_in_mem); //Null segment memory.
		crosspd_memcpy(pagedir, (void*)phdr->load_to, def_cr3(), (uint8_t*)(elf_file)+phdr->data_offset, phdr->size_in_file);
	}

	for(int i=0; i<hdr->sh_ent_cnt; i++) {
		elf_section_header *shdr = (elf_section_header *)((uint8_t*)(elf_file) + hdr->shoff + hdr->sh_ent_size * i);

		if(shdr->addr) {
			crosspd_memcpy(pagedir, (void*)shdr->addr, def_cr3(), (uint8_t*)(elf_file) + shdr->offset, shdr->size > 0x1000 ? 0x1000 : shdr->size);
		}
	}

	result.page_direc = pagedir;
	result.entry_addr = hdr->entry;
	result.success_ld = true;

	return result;
}
