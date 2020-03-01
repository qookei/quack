#include "vmm.h"
#include <mm/mm.h>
#include <mm/pmm.h>
#include <string.h>
#include <kmesg.h>

#include <arch/mm.h>
#include <cpu/cpu.h>
#include <cpu/cpu_data.h>

#include <util/spinlock.h>

#include <loader/elf_common.h>
#include <loader/elf64.h>

#include <util.h>

#include <mm/vm.h>

pt_off_t vmm_virt_to_offs(void *virt) {
	uintptr_t addr = (uintptr_t)virt;

	pt_off_t off = {
		.pml4_off =	(addr & ((size_t)0x1ff << 39)) >> 39,
		.pdp_off =	(addr & ((size_t)0x1ff << 30)) >> 30,
		.pd_off =	(addr & ((size_t)0x1ff << 21)) >> 21,
		.pt_off =	(addr & ((size_t)0x1ff << 12)) >> 12,
	};

	return off;
}

void *vmm_offs_to_virt(pt_off_t offs) {
	uintptr_t addr = 0;

	addr |= offs.pml4_off << 39;
	addr |= offs.pdp_off << 30;
	addr |= offs.pd_off << 21;
	addr |= offs.pt_off << 12;

	return (void *)addr;
}

static pt_t *kernel_pml4;

void vmm_init(void) {
	cpu_set_msr(0x277, 0x0000000005010406);

	kmesg("vmm", "started up!");

	kernel_pml4 = vmm_new_address_space();

	kmesg("vmm", "done creating, setting!");

	vmm_set_context(kernel_pml4);

	kmesg("vmm", "done setting up!");
}

static inline pt_t *vmm_get_or_alloc_ent(pt_t *tab, size_t off, int flags) {
	uint64_t ent_addr = tab->ents[off];
	if (!(ent_addr & VMM_FLAG_PRESENT)) {
		ent_addr = tab->ents[off] = (uint64_t)pmm_alloc(1);
		if (!ent_addr) {
			kmesg("vmm", "failed to allocate a page for a table");
			return NULL;
		}
		tab->ents[off] |= flags | VMM_FLAG_PRESENT;
		memset((void *)(ent_addr + VIRT_PHYS_BASE), 0, 4096);
	}

	return (pt_t *)((ent_addr & VMM_ADDR_MASK) + VIRT_PHYS_BASE);
}

static inline pt_t *vmm_get_or_null_ent(pt_t *tab, size_t off) {
	uint64_t ent_addr = tab->ents[off];
	if (!(ent_addr & VMM_FLAG_PRESENT)) {
		return NULL;
	}

	return (pt_t *)((ent_addr & VMM_ADDR_MASK) + VIRT_PHYS_BASE);
}

int vmm_map_pages(pt_t *pml4, void *virt, void *phys, size_t count, unsigned long perms) {
	int higher_perms = VMM_FLAG_WRITE | VMM_FLAG_USER;

	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_alloc_ent(pml4_virt, offs.pml4_off, higher_perms);
		pt_t *pd_virt = vmm_get_or_alloc_ent(pdp_virt, offs.pdp_off, higher_perms);
		pt_t *pt_virt = vmm_get_or_alloc_ent(pd_virt, offs.pd_off, higher_perms);
		pt_virt->ents[offs.pt_off] = (uint64_t)phys | perms | VMM_FLAG_PRESENT;

		virt = (void *)((uintptr_t)virt + 0x1000);
		phys = (void *)((uintptr_t)phys + 0x1000);
	}

	return 1;
}

int vmm_unmap_pages(pt_t *pml4, void *virt, size_t count) {
	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_null_ent(pml4_virt, offs.pml4_off);
		if (!pdp_virt) return 0;
		pt_t *pd_virt = vmm_get_or_null_ent(pdp_virt, offs.pdp_off);
		if (!pd_virt) return 0;
		pt_t *pt_virt = vmm_get_or_null_ent(pd_virt, offs.pd_off);
		if (!pt_virt) return 0;
		pt_virt->ents[offs.pt_off] = 0;

		virt = (void *)((uintptr_t)virt + 0x1000);
	}

	return 1;
}

int vmm_update_perms(pt_t *pml4, void *virt, size_t count, unsigned long perms) {
	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_null_ent(pml4_virt, offs.pml4_off);
		if (!pdp_virt) return 0;
		pt_t *pd_virt = vmm_get_or_null_ent(pdp_virt, offs.pdp_off);
		if (!pd_virt) return 0;
		pt_t *pt_virt = vmm_get_or_null_ent(pd_virt, offs.pd_off);
		if (!pt_virt) return 0;
		pt_virt->ents[offs.pt_off] = (pt_virt->ents[offs.pt_off] & VMM_ADDR_MASK) | perms | VMM_FLAG_PRESENT;

		virt = (void *)((uintptr_t)virt + 0x1000);
	}

	return 0;
}

int vmm_map_huge_pages(pt_t *pml4, void *virt, void *phys, size_t count, unsigned long perms) {
	int higher_perms = VMM_FLAG_WRITE | VMM_FLAG_USER;

	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_alloc_ent(pml4_virt, offs.pml4_off, higher_perms);
		pt_t *pd_virt = vmm_get_or_alloc_ent(pdp_virt, offs.pdp_off, higher_perms);
		pd_virt->ents[offs.pd_off] = (uint64_t)phys | perms | VMM_FLAG_PRESENT | VMM_FLAG_LARGE;

		virt = (void *)((uintptr_t)virt + 0x200000);
		phys = (void *)((uintptr_t)phys + 0x200000);
	}

	return 1;
}

int vmm_unmap_huge_pages(pt_t *pml4, void *virt, size_t count) {
	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_null_ent(pml4_virt, offs.pml4_off);
		if (!pdp_virt) return 0;
		pt_t *pd_virt = vmm_get_or_null_ent(pdp_virt, offs.pdp_off);
		pd_virt->ents[offs.pd_off] = 0;

		virt = (void *)((uintptr_t)virt + 0x200000);
	}

	return 1;
}

int vmm_update_huge_perms(pt_t *pml4, void *virt, size_t count, unsigned long perms) {
	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_null_ent(pml4_virt, offs.pml4_off);
		if (!pdp_virt) return 0;
		pt_t *pd_virt = vmm_get_or_null_ent(pdp_virt, offs.pdp_off);
		pd_virt->ents[offs.pd_off] = (pd_virt->ents[offs.pd_off] & VMM_ADDR_MASK) | perms | VMM_FLAG_PRESENT | VMM_FLAG_LARGE;

		virt = (void *)((uintptr_t)virt + 0x200000);
	}

	return 0;
}

uintptr_t vmm_get_entry(pt_t *pml4, void *virt) {
	// hack, handle kernel mappings
	if ((uintptr_t)virt >= 0xFFFFFFFF80000000)
		return (uintptr_t)virt - 0xFFFFFFFF80000000; 

	// hack, handle physical memory mapping
	if ((uintptr_t)virt >= 0xFFFF800000000000)
		return (uintptr_t)virt - 0xFFFF800000000000;

	pt_off_t offs = vmm_virt_to_offs(virt);

	pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
	pt_t *pdp_virt = vmm_get_or_null_ent(pml4_virt, offs.pml4_off);
	if (!pdp_virt) return 0;
	pt_t *pd_virt = vmm_get_or_null_ent(pdp_virt, offs.pdp_off);
	if (!pd_virt) return 0;
	pt_t *pt_virt = vmm_get_or_null_ent(pd_virt, offs.pd_off);
	if (!pt_virt) return 0;
	return pt_virt->ents[offs.pt_off];
}

extern void *__ehdr_start;

pt_t *vmm_new_address_space(void) {
	pt_t *new_pml4 = (pt_t *)pmm_alloc(1);
	memset((void *)((uintptr_t)new_pml4 + VIRT_PHYS_BASE), 0, 4096);

	if (kernel_pml4) {
		memcpy(
			(void *)((uintptr_t)new_pml4 + 2048 + VIRT_PHYS_BASE),
			(void *)((uintptr_t)kernel_pml4 + 2048 + VIRT_PHYS_BASE),
			2048);
		return new_pml4;
	}

	vmm_map_huge_pages(new_pml4, (void *)0xFFFF800000000000, NULL, 512 * 4, 3);

	elf64_ehdr *ehdr = (elf64_ehdr *)(&__ehdr_start);

	if (!ehdr) {
		vmm_map_huge_pages(new_pml4, (void *)0xFFFFFFFF80000000, NULL, 64, 3);
		return new_pml4;
	}

	elf64_phdr *phdrs = (elf64_phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

	for (size_t i = 0; i < ehdr->e_phnum; i++) {
		if (phdrs[i].p_type != ELF_PHDR_TYPE_LOAD)
			continue;

		// discard the LOAD segment that contains SMP code
		if (phdrs[i].p_vaddr < 0xFFFFFFFF80000000ULL)
			continue;

		unsigned long flags = VMM_FLAG_NX | VMM_FLAG_PRESENT;

		if (phdrs[i].p_flags & ELF_PHDR_FLAG_X) {
			flags &= ~VMM_FLAG_NX;
		}

		if (phdrs[i].p_flags & ELF_PHDR_FLAG_W) {
			flags |= VMM_FLAG_WRITE;
		}

		size_t n_pages = (phdrs[i].p_memsz + 0xFFF) / 0x1000;

		vmm_map_pages(new_pml4,
			(void *)phdrs[i].p_vaddr,
			(void *)phdrs[i].p_paddr,
			n_pages, flags);
	}

	return new_pml4;
}

pt_t **get_ctx_ptr(void) {
	return (pt_t **)&cpu_data_get(cpu_get_id())->saved_vmm_ctx;
}

void vmm_save_context(void) {
	pt_t **ctx = get_ctx_ptr();
	if (*ctx)
		kmesg("vmm", "vmm_save_context will overwrite the saved context! saved context: %016p", *ctx);
	*ctx = vmm_get_current_context();
}

pt_t *vmm_get_saved_context(void) {
	pt_t **ctx = get_ctx_ptr();
	return *ctx;
}

void vmm_restore_context(void) {
	pt_t **ctx = get_ctx_ptr();

	if (!*ctx)
		kmesg("vmm", "no context to restore in vmm_restore_context");

	vmm_set_context(*ctx);
	*ctx = NULL;
}

void vmm_drop_context(void) {
	pt_t **ctx = get_ctx_ptr();
	*ctx = NULL;
}

void vmm_set_context(pt_t *ctx) {
	asm volatile ("mov %%rax, %%cr3" : : "a"(ctx) : "memory");
}

pt_t *vmm_get_current_context(void) {
	uintptr_t ctx = 0;
	asm volatile ("mov %%cr3, %%rax" : "=a"(ctx) : : "memory");
	return (pt_t *)ctx;
}

void vmm_update_mapping(void *ptr) {
	asm volatile ("invlpg (%0)" : : "r"(ptr) : "memory");
}

spinlock_t copy_lock;

#define MAX(x, y) ((x) > (y) ? (x) : (y))

void vmm_ctx_memcpy(pt_t *dst_ctx, void *dst_addr, pt_t *src_ctx, void *src_addr, size_t size) {
	// map dst to 0x700000000000
	// map src to 0x780000000000
	// memcpy from mapped src to mapped dst
	// unmap

	// max amount of data you can copy is 8TB if aligned to a page

	spinlock_lock(&copy_lock);

	uintptr_t src_virt = 0x780000000000;
	uintptr_t dst_virt = 0x700000000000;
	uintptr_t dst = (uintptr_t)dst_addr & (~0xFFF);
	uintptr_t src = (uintptr_t)src_addr & (~0xFFF);

	size_t map_size = size + MAX(
			(uintptr_t)dst_addr & 0xFFF,
			(uintptr_t)src_addr & 0xFFF);

	for (size_t i = 0; i < map_size;
		i += 0x1000, src_virt += 0x1000, src += 0x1000,
			dst_virt += 0x1000, dst += 0x1000) {
		uintptr_t dst_phys = vmm_get_entry(dst_ctx, (void *)dst)
				& VMM_ADDR_MASK;
		uintptr_t src_phys = vmm_get_entry(src_ctx, (void *)src)
				& VMM_ADDR_MASK;

		vmm_map_pages(kernel_pml4, (void *)dst_virt, (void *)dst_phys,
					1, VMM_FLAG_WRITE);
		vmm_map_pages(kernel_pml4, (void *)src_virt, (void *)src_phys,
					1, VMM_FLAG_WRITE);
		vmm_update_mapping((void *)dst_virt);
		vmm_update_mapping((void *)src_virt);
	}

	memcpy((void *)(0x700000000000 + ((uintptr_t)dst_addr & 0xFFF)),
		(void *)(0x780000000000 + ((uintptr_t)src_addr & 0xFFF)), size);

	src_virt = 0x780000000000;
	dst_virt = 0x700000000000;
	for (size_t i = 0; i < map_size;
		i += 0x1000, src_virt += 0x1000, dst_virt += 0x1000) {
		vmm_unmap_pages(kernel_pml4, (void *)dst_virt, 1);
		vmm_unmap_pages(kernel_pml4, (void *)src_virt, 1);
		vmm_update_mapping((void *)dst_virt);
		vmm_update_mapping((void *)src_virt);
	}

	spinlock_release(&copy_lock);
}

int vmm_arch_to_vmm_flags(int flags, int cache) {
	int arch_flags = 0;

	if (flags & vm_perm::write) arch_flags |= VMM_FLAG_WRITE;
	if (flags & vm_perm::user) arch_flags |= VMM_FLAG_USER;
	if (!(flags & vm_perm::execute)) arch_flags |= VMM_FLAG_NX;

	if (cache & (1 << 0)) arch_flags |= VMM_FLAG_PAT0;
	if (cache & (1 << 1)) arch_flags |= VMM_FLAG_PAT1;
	if (cache & (1 << 2)) arch_flags |= VMM_FLAG_PAT2;

	return arch_flags;
}

// arch functions
// TODO: add locking

int arch_mm_map_kernel(void *dst, void *src, size_t size, int flags, int cache) {
	return vmm_map_pages(kernel_pml4, dst, src, size, vmm_arch_to_vmm_flags(flags, cache));
}

int arch_mm_unmap_kernel(void *dst, size_t size) {
	return vmm_unmap_pages(kernel_pml4, dst, size);
}

int arch_mm_map(void *ctx, void *dst, void *src, size_t size, int flags, int cache) {
	return vmm_map_pages((pt_t *)ctx, dst, src, size, vmm_arch_to_vmm_flags(flags, cache));
}

int arch_mm_unmap(void *ctx, void *dst, size_t size) {
	return vmm_unmap_pages((pt_t *)ctx, dst, size);
}

uintptr_t arch_mm_get_phys_kernel(void *dst) {
	return vmm_get_entry(kernel_pml4, dst) & VMM_ADDR_MASK;
}

int arch_mm_get_flags_kernel(void *dst) {
	int arch_flags = vmm_get_entry(kernel_pml4, dst) & VMM_FLAG_MASK;
	int flags = arch_flags ? vm_perm::read : 0;
	if (arch_flags & VMM_FLAG_WRITE) flags |= vm_perm::write;
	if (arch_flags & VMM_FLAG_USER) flags |= vm_perm::user;
	if (!(arch_flags & VMM_FLAG_NX)) flags |= vm_perm::execute;
	return flags;
}

int arch_mm_get_cache_kernel(void *dst) {
	int arch_flags = vmm_get_entry(kernel_pml4, dst) & VMM_FLAG_MASK;
	int flags = 0;
	if (arch_flags & VMM_FLAG_PAT0) flags |= (1 << 0);
	if (arch_flags & VMM_FLAG_PAT1) flags |= (1 << 1);
	if (arch_flags & VMM_FLAG_PAT2) flags |= (1 << 2);
	return flags;
}

void *arch_mm_get_ctx_kernel(void) {
	return kernel_pml4;
}

int arch_mm_store_context(void) {
	vmm_save_context();
	return 1;
}

int arch_mm_switch_context(void *ctx) {
	vmm_set_context((pt_t *)ctx);
	return 1;
}

int arch_mm_restore_context(void) {
	vmm_restore_context();
	return 1;
}

int arch_mm_drop_context(void) {
	vmm_drop_context();
	return 1;
}

// -1 for this CPU
// -2 for all CPUs
int arch_mm_update_context_all(int cpu) {
	(void)cpu; // TODO: handle this
	pt_t *ctx = vmm_get_current_context();
	vmm_set_context(ctx);
	return 1;
}

int arch_mm_update_context_single(int cpu, void *dst) {
	(void)cpu; // TODO: handle this
	vmm_update_mapping(dst);
	return 1;
}

void *arch_mm_create_context(void) {
	return vmm_new_address_space();
}

void arch_mm_destroy_context(void *ctx) {
	pt_t *pml4_virt = (pt_t *)((uint64_t)ctx + VIRT_PHYS_BASE);

	for (int i = 0; i < 256; i++) {
		pt_t *pdp_virt = vmm_get_or_null_ent(pml4_virt, i);
		if (!pdp_virt) continue;

		for (int j = 0; j < 512; j++) {
			pt_t *pd_virt = vmm_get_or_null_ent(pdp_virt, j);
			if (!pd_virt) continue;

			for (int k = 0; k < 512; k++) {
				if (pd_virt->ents[k] & VMM_FLAG_LARGE)
					continue;
				if (pd_virt->ents[k] & VMM_FLAG_PRESENT)
					continue;

				pmm_free((void *)(pd_virt->ents[k] & VMM_ADDR_MASK), 1);
			}

			pmm_free((void *)(pdp_virt->ents[j] & VMM_ADDR_MASK), 1);
		}

		pmm_free((void *)(pml4_virt->ents[i] & VMM_ADDR_MASK), 1);
	}

	pmm_free(ctx, 1);
}

void arch_mm_mapping_load(memory_mapping *mapping, ptrdiff_t offset, void *data, size_t size) {
	spinlock_lock(&copy_lock);

	uintptr_t virt = 0x700000000000;
	for (size_t i = 0; i < mapping->_size; i++, virt += 0x1000) {
		uintptr_t phys = mapping->_backing_pages[i]._ptr;

		vmm_map_pages(kernel_pml4, (void *)virt, (void *)phys,
					1, VMM_FLAG_WRITE);
	}

	memcpy((void *)(0x700000000000 + offset), data, size);

	virt = 0x700000000000;
	for (size_t i = 0; i < mapping->_size; i++, virt += 0x1000) {
		vmm_unmap_pages(kernel_pml4, (void *)virt, 1);
		vmm_update_mapping((void *)virt);
	}

	spinlock_release(&copy_lock);
}

void arch_mm_mapping_store(memory_mapping *mapping, ptrdiff_t offset, void *data, size_t size) {
	spinlock_lock(&copy_lock);

	uintptr_t virt = 0x700000000000;
	for (size_t i = 0; i < mapping->_size; i++, virt += 0x1000) {
		uintptr_t phys = mapping->_backing_pages[i]._ptr;

		vmm_map_pages(kernel_pml4, (void *)virt, (void *)phys,
					1, VMM_FLAG_WRITE);
	}

	memcpy(data, (void *)(0x700000000000 + offset), size);

	virt = 0x700000000000;
	for (size_t i = 0; i < mapping->_size; i++, virt += 0x1000) {
		vmm_unmap_pages(kernel_pml4, (void *)virt, 1);
		vmm_update_mapping((void *)virt);
	}

	spinlock_release(&copy_lock);
}
