#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

#define VMM_FLAG_MASK (0xFFF | (1ull << 63))
#define VMM_ADDR_MASK ~(VMM_FLAG_MASK)

#define VMM_FLAG_PRESENT	(1ull<<0)
#define VMM_FLAG_WRITE		(1ull<<1)
#define VMM_FLAG_USER		(1ull<<2)
#define VMM_FLAG_PAT0		(1ull<<3)
#define VMM_FLAG_PAT1		(1ull<<4)
#define VMM_FLAG_PAT2		(1ull<<7)
#define VMM_FLAG_DIRTY		(1ull<<5)
#define VMM_FLAG_LARGE		(1ull<<7)
#define VMM_FLAG_NX		(1ull<<63)

typedef struct {
	uint64_t ents[512];
} pt_t;

typedef struct {
	size_t pml4_off;
	size_t pdp_off;
	size_t pd_off;
	size_t pt_off;
} pt_off_t;

pt_off_t vmm_virt_to_offs(void *);
void *vmm_offs_to_virt(pt_off_t);

void vmm_init(void);

int vmm_map_pages(pt_t *, void *, void *, size_t, unsigned long);
int vmm_unmap_pages(pt_t *, void *, size_t);
int vmm_update_perms(pt_t *, void *, size_t, unsigned long);

uintptr_t vmm_get_entry(pt_t *, void *);

// creates an address space with the kernel and physical memory mappings
pt_t *vmm_new_address_space(void);

void vmm_save_context(void);
pt_t *vmm_get_saved_context(void);
void vmm_restore_context(void);
void vmm_drop_context(void);
void vmm_set_context(pt_t *);
pt_t *vmm_get_current_context(void);

int vmm_arch_to_vmm_flags(int, int);

void vmm_ctx_memcpy(pt_t *, void *, pt_t *, void *, size_t);

#endif
