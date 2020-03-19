#include "object.h"
#include <util.h>

object_group::object_group(base_object *parent)
: _parent{parent}, _objects{frg::hash<uint64_t>{}, frg_allocator::get()},
_members{frg::hash<uint64_t>{}, frg_allocator::get()}, _next_id{0}, _lock{} {
}

object_group::~object_group() {
	for (auto curr = _objects.begin(); curr != _objects.end();) {
		auto t = curr;
		++curr;

		delete t->get<1>();
	}
}

handle_t object_group::add_object(base_object *obj) {
	if (!_dont_lock)
		_lock.lock();

	handle_t h = _next_id.load();
	_next_id.fetch_add(1);

	_objects[h] = obj;
	kmesg("group", "obj=%p", obj);
	obj->set_parent(this);

	if (!_dont_lock)
		_lock.unlock();

	return h;
}

void object_group::remove_object(handle_t handle) {
	if (!_dont_lock)
		_lock.lock();

	auto item = _objects.remove(handle);
	assert(item);

	delete *item;

	if (!_dont_lock)
		_lock.unlock();
}

base_object *object_group::get_object(handle_t handle) {
	return _objects[handle];
}

bool object_group::check_membership(uint64_t id) {
	return _members[id];
}

void object_group::add_member(uint64_t id) {
	if (!_dont_lock)
		_lock.lock();

	_members[id] = true;

	if (!_dont_lock)
		_lock.unlock();
}

void object_group::remove_member(uint64_t id) {
	if (!_dont_lock)
		_lock.lock();

	_members[id] = false;

	if (!_dont_lock)
		_lock.unlock();
}
