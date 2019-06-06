#ifndef ARCH_MM_H
#define ARCH_MM_H

#include <stdint.h>
#include <stddef.h>

#define ARCH_MM_FLAGS_READ			0x01		/* Page is readable */
#define ARCH_MM_FLAGS_WRITE			0x02		/* Page is writable */
#define ARCH_MM_FLAGS_EXECUTE		0x04		/* Page is executable */
#define ARCH_MM_FLAGS_USER			0x08		/* Page is accessible in a lesser privileged mode(user) */
#define ARCH_MM_FLAGS_NO_CACHE		0x10		/* Page is not cached */

#if ARCH == i386 || ARCH == x86_64
#define ARCH_MM_PAGE_SIZE 0x1000
#endif

int arch_mm_map_kernel(int cpu, void *dst, void *src, size_t size, int flags);
int arch_mm_unmap_kernel(int cpu, void *dst, size_t size);
uintptr_t arch_mm_get_phys_kernel(int cpu, void *dst);
int arch_mm_get_flags_kernel(int cpu, void *dst);

/* TODO: add this once we have at least generic definitions for task stuff
	int arch_mm_map_user(task_t *task, void *dst, void *src, size_t size, int flags);
	int arch_mm_unmap_user(task_t *task, void *dst, size_t size);
	uintptr_t arch_mm_get_phys_user(task_t *task, void *dst);
	int arch_mm_get_flags_user(task_t *task, void *dst);
*/

void *arch_mm_alloc_phys(size_t blocks);
int arch_mm_free_phys(void *mem, size_t blocks);

void *arch_mm_get_ctx_kernel(int cpu);

int arch_mm_store_context(void);
int arch_mm_switch_context(void *ctx);
int arch_mm_restore_context(void);

// -1 for this CPU
// -2 for all CPUs
int arch_mm_update_context_all(int cpu);
int arch_mm_update_context_single(int cpu, void *dst);

#endif // ARCH_MM_H
