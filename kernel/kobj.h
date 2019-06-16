#ifndef KOBJ_H
#define KOBJ_H

#include <stdint.h>
#include <stddef.h>

#define KOBJ_PERM_ALLOWED_CHECK		(1 << 0)
#define KOBJ_PERM_ALLOWED_READ		(1 << 1)
#define KOBJ_PERM_ALLOWED_WRITE		(1 << 2)
#define KOBJ_PERM_ALLOWED_USE		(1 << 3)

#define KOBJ_PERM_OTHER_CHECK		(1 << 4)
#define KOBJ_PERM_OTHER_READ		(1 << 5)
#define KOBJ_PERM_OTHER_WRITE		(1 << 6)
#define KOBJ_PERM_OTHER_USE		(1 << 7)

// generic permissions
#define KOBJ_PERM_CHECK		(1 << 0)
#define KOBJ_PERM_READ		(1 << 1)
#define KOBJ_PERM_WRITE		(1 << 2)
#define KOBJ_PERM_USE		(1 << 3)
#define KOBJ_PERM_MANAGE	(1 << 4)

typedef uint64_t handle_t;

typedef struct {
	int32_t pid;
	int perms;
} kobj_allowed_t;

typedef struct {
	int32_t owner_pid;
	char *name;

	int perms;

	kobj_allowed_t *allowed_pids;
	size_t n_allowed_pids;

	size_t len;
	void *data;
} kobj_t;

void kobj_init(void);

kobj_t *kobj_get(handle_t hnd);

handle_t kobj_create_new(int32_t owner, char *name, int perms, size_t len);
int kobj_destroy(handle_t hnd);

int kobj_has_perm(handle_t hnd, int32_t this_pid, int perms);

int kobj_allow_process(handle_t hnd, int32_t pid, int perms);
int kobj_disallow_process(handle_t hnd, int32_t pid);

#endif
