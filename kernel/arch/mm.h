#ifndef ARCH_MM_H
#define ARCH_MM_H

#include <stdint.h>
#include <stddef.h>

#if ARCH == i386 || ARCH == x86_64
constexpr inline size_t vm_page_size = 0x1000;
constexpr inline uintptr_t vm_user_base = 0x1000;
#endif

#if ARCH == x86_64
constexpr inline size_t vm_heap_base = 0x200000000;
constexpr inline uintptr_t vm_user_end = 0x800000000000;
#endif

namespace vm_perm {
	constexpr inline int read = 1;
	constexpr inline int write = 2;
	constexpr inline int execute = 4;
	constexpr inline int user = 8;

	constexpr inline int ro = read;
	constexpr inline int rw = read | write;
	constexpr inline int rx = read | execute;
	constexpr inline int rwx = read | write | execute;

	constexpr inline int uro = user | read;
	constexpr inline int urw = user | read | write;
	constexpr inline int urx = user | read | execute;
	constexpr inline int urwx = user | read | write | execute;
}

namespace vm_cache {
	constexpr inline int wb = 0;
	constexpr inline int wt = 1;
	constexpr inline int wc = 2;
	constexpr inline int wp = 3;
	constexpr inline int uc = 4;
	constexpr inline int def = wb;
}

int arch_mm_map_kernel(void *dst, void *src, size_t size, int flags, int cache);
int arch_mm_unmap_kernel(void *dst, size_t size);
int arch_mm_map(void *ctx, void *dst, void *src, size_t size, int flags, int cache);
int arch_mm_unmap(void *ctx, void *dst, size_t size);

uintptr_t arch_mm_get_phys_kernel(void *dst);
int arch_mm_get_flags_kernel(void *dst);
int arch_mm_get_cache_kernel(void *dst);

void *arch_mm_alloc_phys(size_t blocks);
int arch_mm_free_phys(void *mem, size_t blocks);

void *arch_mm_get_ctx_kernel(void);

int arch_mm_store_context(void);
int arch_mm_switch_context(void *ctx);
int arch_mm_restore_context(void);
int arch_mm_drop_context(void);
void *arch_mm_create_context(void);

// -1 for this CPU
// -2 for all CPUs
int arch_mm_update_context_all(int cpu);
int arch_mm_update_context_single(int cpu, void *dst);

#endif // ARCH_MM_H
