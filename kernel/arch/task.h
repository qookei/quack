#ifndef ARCH_TASK_T
#define ARCH_TASK_T

#include <stddef.h>
#include <stdint.h>

struct arch_task_;
typedef struct arch_task_ arch_task_t;

// task management
arch_task_t *arch_task_create_new(void *mem_ctx);
int arch_task_destroy(arch_task_t *task);

// state change
__attribute__((noreturn))
void arch_task_switch_to(arch_task_t *task);
int arch_task_save_irq_state(arch_task_t *task, void *irq_state);
int arch_task_load_stack_ptr(arch_task_t *task, uintptr_t stack_ptr);
int arch_task_load_entry_point(arch_task_t *task, uintptr_t entry);

// permission management (not required)
// - ports
__attribute__((weak))
int arch_task_allow_port_access(arch_task_t *task, uintptr_t port, size_t size);
__attribute__((weak))
int arch_task_disallow_port_access(arch_task_t *task, uintptr_t port, size_t size);

// - memory
struct mem_region {
	uintptr_t dest;

	uintptr_t start;
	uintptr_t end;
};
__attribute__((weak)) uintptr_t arch_task_allow_mem_access(arch_task_t *task, struct mem_region *region);
__attribute__((weak)) int arch_task_disallow_mem_access(arch_task_t *task, struct mem_region *region);

// context information
void *arch_task_get_mem_context(arch_task_t *task);

// memory management
int arch_task_alloc_mem_region(arch_task_t *task, struct mem_region *region, int flags);
int arch_task_copy_to_mem_region(arch_task_t *task, struct mem_region *region, void *src, size_t count);
int arch_task_free_mem_region(arch_task_t *task, struct mem_region *region);

#endif
