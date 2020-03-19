#ifndef PROC_GROUPS_H
#define PROC_GROUPS_H

#include <frg/eternal.hpp>
#include <frg/hash_map.hpp>
#include <object/object.h>
#include <spinlock.h>
#include <type_traits>
#include <function_traits.h>

struct global_object_groups {
	friend class frg::eternal<global_object_groups>;

	static global_object_groups &get() {
		static frg::eternal<global_object_groups> _inst;
		return _inst.get();
	}

	template <typename F, typename ...Args>
	auto check_membership_and(uint64_t t, uint64_t id, F func, Args &&...args) {
		auto grp = _groups.get(id);

		assert(grp);

		frg::unique_lock guard{grp->lock};

		assert(grp->grp);

		return grp->grp->check_membership_and(t, func, std::forward<Args...>(args...));
	}

	handle_t new_group(uint64_t t) {
		frg::unique_lock guard{_lock};

		handle_t id = _next_id++;

		_groups.insert(id, {new object_group, {}});
		_groups[id].grp->add_member(t);

		return id;
	}

	bool remove_group(uint64_t t, handle_t id) {
		frg::unique_lock main_guard{_lock};

		auto grp = _groups.get(id);

		if (!grp)
			return false;

		frg::unique_lock guard{grp->lock};

		if (!grp->grp)
			return false;

		if (!grp->grp->check_membership(t))
			return false;

		delete _groups[id].grp;
		return true;
	}

private:
	global_object_groups()
	:_groups{frg::hash<uint64_t>{}, frg_allocator::get()}, _next_id{0} {}

	struct locked_group {
		object_group *grp;
		spinlock lock;
	};

	frg::hash_map<
		uint64_t,
		locked_group,
		frg::hash<uint64_t>,
		frg_allocator
	> _groups;

	uint64_t _next_id;

	spinlock _lock;

};

#endif //PROC_GROUPS_H
