#include "object.h"
#include <util.h>

ipc_queue::ipc_queue(base_object *parent)
: _parent{parent}, _has_new{false}, _n_messages{0},
_buffer{frg_allocator::get()}, _enqueue{0}, _dequeue{0} {
}

ipc_queue::~ipc_queue() {
}

void ipc_queue::send(frg::unique_ptr<ipc_queue::message, frg_allocator> &&message) {
	if (_buffer.size() == _n_messages) {
		_buffer.emplace_back(frg_allocator::get());
	}

	assert(!_buffer[_enqueue]);

	_buffer[_enqueue] = std::move(message);

	if (++_enqueue >= _buffer.size())
		_enqueue = 0;

	_has_new = true;
	_n_messages++;
}

ipc_queue::message &ipc_queue::peek() {
	assert(_buffer[_dequeue]);
	return *_buffer[_dequeue];
}

frg::unique_ptr<ipc_queue::message, frg_allocator> ipc_queue::pop() {
	using std::swap;

	assert(_buffer[_dequeue]);

	frg::unique_ptr<ipc_queue::message, frg_allocator> item{frg_allocator::get()};
	swap(_buffer[_dequeue], item);
	_n_messages--;
	return item;
}

bool ipc_queue::has_new() {
	bool had_new = _has_new;
	_has_new = false;

	return had_new;
}

size_t ipc_queue::get_queue_size() {
	return _n_messages;
}

