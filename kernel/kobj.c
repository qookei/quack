#include "kobj.h"
#include <string.h>
#include <mm/heap.h>
#include <panic.h>
#include <util.h>
#include <kmesg.h>

static kobj_t **objects = NULL;
static size_t n_objects = 0;

kobj_t *kobj_get(handle_t hnd) {
	if (hnd < n_objects)
		return objects[hnd];

	kmesg("kobj", "access to invalid handle %lu", hnd);
	return NULL;
}

static handle_t kobj_alloc_new(void) {
	if (!objects) {
		n_objects = 16;
		objects = kcalloc(sizeof (kobj_t *), 16);
	}

	handle_t hnd = 1;
	while (hnd < n_objects && objects[hnd])
		hnd++;

	if (hnd == n_objects) {	// no free slots
		size_t old_n_objects = n_objects;

		n_objects *= 2;
		objects = krealloc(objects, sizeof (kobj_t *) * n_objects);
		memset(&objects[old_n_objects], 0, old_n_objects * sizeof(kobj_t *));
	}

	objects[hnd] = kcalloc(sizeof(kobj_t), 1);

	return hnd;
}

handle_t kobj_create_new(int32_t owner, char *name, int perms, size_t len) {
	handle_t hnd = kobj_alloc_new();
	kobj_t *obj = kobj_get(hnd);

	assert(obj);
	//if (!obj)
	//	return 0;

	obj->owner_pid = owner;

	if (name) {
		obj->name = kmalloc(strlen(name) + 1);
		strcpy(obj->name, name);
	} else {
		obj->name = NULL;
	}

	obj->perms = perms;

	obj->n_allowed_pids = 0;
	obj->allowed_pids = NULL;

	obj->len = len;
	obj->data = kcalloc(len, 1);

	return hnd;
}

int kobj_destroy(handle_t hnd) {
	kobj_t *obj = kobj_get(hnd);

	assert(obj);
	//if (!obj)
	//	return 0;

	if (obj->name)
		kfree(obj->name);

	if (obj->data)
		kfree(obj->data);

	kfree(obj);

	objects[hnd] = NULL;

	return 1;
}

int kobj_has_perm(handle_t hnd, int32_t this_pid, int perms) {
	kobj_t *obj = kobj_get(hnd);

	assert(obj);
	//if (!obj)
	//	return 0;

	if (!this_pid) // kernel is "pid 0" and has access to everything
		return 1;

	if (obj->owner_pid == this_pid) // owner can do anything
		return 1;

	if ((perms & KOBJ_PERM_CHECK) &&
		(obj->perms & KOBJ_PERM_OTHER_CHECK))
		return 1;

	if ((perms & KOBJ_PERM_READ) &&
		(obj->perms & KOBJ_PERM_OTHER_READ))
		return 1;

	if ((perms & KOBJ_PERM_WRITE) &&
		(obj->perms & KOBJ_PERM_OTHER_WRITE))
		return 1;

	if ((perms & KOBJ_PERM_USE) &&
		(obj->perms & KOBJ_PERM_OTHER_USE))
		return 1;

	for (size_t i = 0; i < obj->n_allowed_pids; i++) {
		kobj_allowed_t a = obj->allowed_pids[i];

		if (!a.pid) // this entry is empty
			continue;

		if (a.pid != this_pid)
			continue;

		if ((perms & KOBJ_PERM_CHECK) &&
			(obj->perms & KOBJ_PERM_ALLOWED_CHECK))
			return 1;

		if ((perms & KOBJ_PERM_READ) &&
			(obj->perms & KOBJ_PERM_ALLOWED_READ))
			return 1;

		if ((perms & KOBJ_PERM_WRITE) &&
			(obj->perms & KOBJ_PERM_ALLOWED_WRITE))
			return 1;

		if ((perms & KOBJ_PERM_USE) &&
			(obj->perms & KOBJ_PERM_ALLOWED_USE))
			return 1;

		if ((perms & a.perms) == perms)
			return 1;
	}

	return 0;
}

int kobj_allow_process(handle_t hnd, int32_t pid, int perms) {
	kobj_t *obj = kobj_get(hnd);

	assert(obj);
	//if (!obj)
	//	return 0;

	if (!obj->n_allowed_pids) {
		obj->n_allowed_pids = 1;
		obj->allowed_pids = kcalloc(sizeof(int32_t), 1);
	}

	size_t i = 0;
	while (i < obj->n_allowed_pids && obj->allowed_pids[i].pid &&
			obj->allowed_pids[i].pid != pid)
		i++;

	if (i == obj->n_allowed_pids) {
		size_t old = obj->n_allowed_pids;
		obj->n_allowed_pids *= 2;
		obj->allowed_pids = krealloc(obj->allowed_pids, obj->n_allowed_pids * sizeof(kobj_allowed_t));
		memset(&obj->allowed_pids[old], 0, old * sizeof(kobj_allowed_t));
	}

	obj->allowed_pids[i].pid = pid;
	obj->allowed_pids[i].perms |= perms;

	kmesg("kobj", "allowed process %d to use object %lu", pid, hnd);

	return 1;
}

int kobj_disallow_process(handle_t hnd, int32_t pid) {
	kobj_t *obj = kobj_get(hnd);

	assert(obj);
	//if (!obj)
	//	return 0;

	if (!obj->n_allowed_pids)
		return 0;

	size_t i = 0;
	while (i < obj->n_allowed_pids && obj->allowed_pids[i].pid &&
			obj->allowed_pids[i].pid != pid)
		i++;

	if (i == obj->n_allowed_pids)
		return 0;

	if (obj->allowed_pids[i].pid != pid)
		return 0;

	obj->allowed_pids[i].pid = 0;
	obj->allowed_pids[i].perms = 0;

	kmesg("kobj", "disallowed process %d from accessing object %lu", pid, hnd);

	return 1;
}
