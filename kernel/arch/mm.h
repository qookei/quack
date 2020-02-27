#ifndef ARCH_MM_H
#define ARCH_MM_H

#include <stdint.h>
#include <stddef.h>

#define ARCH_MM_FLAG_R		0x01 /* page is readable */
#define ARCH_MM_FLAG_W		0x02 /* page is writable */
#define ARCH_MM_FLAG_E		0x04 /* page is executable */
#define ARCH_MM_FLAG_U		0x08 /* page is accessible in user mode */

#define ARCH_MM_CACHE_WB	0 /* write-back */
#define ARCH_MM_CACHE_WT	1 /* write-through */
#define ARCH_MM_CACHE_WC	2 /* write-combining */
#define ARCH_MM_CACHE_WP	3 /* write-protect */
#define ARCH_MM_CACHE_UC	4 /* uncacheable */
#define ARCH_MM_CACHE_DEFAULT ARCH_MM_CACHE_WB

#if ARCH == i386 || ARCH == x86_64
#define ARCH_MM_PAGE_SIZE 0x1000
#endif

#if ARCH == x86_64
#define ARCH_MM_HEAP_BASE 0x200000000
#endif

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
