#include "vmm.h"
#include <mm/mm.h>
#include <mm/pmm.h>
#include <string.h>
#include <kmesg.h>

#include <arch/mm.h>
#include <cpu/cpu.h>
#include <cpu/cpu_data.h>

#include <util/spinlock.h>

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

// TODO: multicore?
static pt_t *kernel_pml4;

void vmm_init(void) {
	kmesg("vmm", "started up!");
	
	kernel_pml4 = vmm_new_address_space();

	kmesg("vmm", "done creating, setting!");
	
	vmm_set_context(kernel_pml4);

	kmesg("vmm", "done setting up!");
}

static inline pt_t *vmm_get_or_alloc_ent(pt_t *tab, size_t off, int flags) {
	uint64_t ent_addr = tab->ents[off] & VMM_ADDR_MASK;
	if (!ent_addr) {
		ent_addr = tab->ents[off] = (uint64_t)pmm_alloc(1);
		if (!ent_addr) {
			kmesg("vmm", "failed to allocate a page for a table");
			return NULL;
		}
		tab->ents[off] |= flags | VMM_FLAG_PRESENT;
		memset((void *)(ent_addr + VIRT_PHYS_BASE), 0, 4096);
	}

	return (pt_t *)(ent_addr + VIRT_PHYS_BASE);
}

static inline pt_t *vmm_get_or_null_ent(pt_t *tab, size_t off) {
	uint64_t ent_addr = tab->ents[off] & VMM_ADDR_MASK;
	if (!ent_addr) {
		return NULL;
	}

	return (pt_t *)(ent_addr + VIRT_PHYS_BASE);
}

int vmm_map_pages(pt_t *pml4, void *virt, void *phys, size_t count, int perms) {
	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_alloc_ent(pml4_virt, offs.pml4_off, perms);
		pt_t *pd_virt = vmm_get_or_alloc_ent(pdp_virt, offs.pdp_off, perms);
		pt_t *pt_virt = vmm_get_or_alloc_ent(pd_virt, offs.pd_off, perms);
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

int vmm_update_perms(pt_t *pml4, void *virt, size_t count, int perms) {
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

int vmm_map_huge_pages(pt_t *pml4, void *virt, void *phys, size_t count, int perms) {
	while (count--) {
		pt_off_t offs = vmm_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = vmm_get_or_alloc_ent(pml4_virt, offs.pml4_off, perms);
		pt_t *pd_virt = vmm_get_or_alloc_ent(pdp_virt, offs.pdp_off, perms);
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

int vmm_update_huge_perms(pt_t *pml4, void *virt, size_t count, int perms) {
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

pt_t *vmm_new_address_space(void) {
	pt_t *new_pml4 = pmm_alloc(1);
	memset((void *)((uintptr_t)new_pml4 + VIRT_PHYS_BASE), 0, 4096);

	vmm_map_huge_pages(new_pml4, (void *)0xFFFFFFFF80000000, NULL, 64, 3);
	vmm_map_huge_pages(new_pml4, (void *)0xFFFF800000000000, NULL, 512 * 4, 3);

	return new_pml4;
}

pt_t *vmm_clone_address_space(pt_t *addr) {
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

int vmm_arch_to_vmm_flags(int flags) {
	return ((flags & ARCH_MM_FLAGS_WRITE) ? VMM_FLAG_WRITE : 0)
		| ((flags & ARCH_MM_FLAGS_USER) ? VMM_FLAG_USER : 0)
		| ((flags & ARCH_MM_FLAGS_NO_CACHE) ?
			(VMM_FLAG_NO_CACHE | VMM_FLAG_WT) : 0);

	// TODO: add EXECUTE permission bit support
}

// arch functions
// TODO: add locking

int arch_mm_map_kernel(int cpu, void *dst, void *src, size_t size, int flags) {
	(void)cpu; // TODO: ??

	return vmm_map_pages(kernel_pml4, dst, src, size, vmm_arch_to_vmm_flags(flags));
}

int arch_mm_unmap_kernel(int cpu, void *dst, size_t size) {
	(void)cpu;
	return vmm_unmap_pages(kernel_pml4, dst, size);
}

uintptr_t arch_mm_get_phys_kernel(int cpu, void *dst) {
	(void)cpu;
	return vmm_get_entry(kernel_pml4, dst) & VMM_ADDR_MASK;
}

int arch_mm_get_flags_kernel(int cpu, void *dst) {
	(void)cpu;
	int arch_flags = vmm_get_entry(kernel_pml4, dst) & VMM_FLAG_MASK;
	int flags = arch_flags ? (ARCH_MM_FLAGS_READ | ARCH_MM_FLAGS_EXECUTE) : 0;
	if (arch_flags & VMM_FLAG_WRITE) flags |= ARCH_MM_FLAGS_WRITE;
	if (arch_flags & VMM_FLAG_USER) flags |= ARCH_MM_FLAGS_USER;
	if (arch_flags & VMM_FLAG_NO_CACHE) flags |= ARCH_MM_FLAGS_NO_CACHE;
	if (arch_flags & VMM_FLAG_WT) flags |= ARCH_MM_FLAGS_NO_CACHE;
	return flags;
}

void *arch_mm_get_ctx_kernel(int cpu) {
	(void)cpu;
	return kernel_pml4;
}

int arch_mm_store_context(void) {
	vmm_save_context();
	return 1;
}

int arch_mm_switch_context(void *ctx) {
	vmm_set_context(ctx);
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
