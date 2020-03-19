#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

#include <new>
#include <lib/frg_allocator.h>
#include <frg/vector.hpp>
#include <atomic>

#include <frg/hash.hpp>
#include <frg/hash_map.hpp>
#include <frg/unique.hpp>
#include <frg/optional.hpp>
#include <frg/mutex.hpp>

#include <spinlock.h>

#include <function_traits.h>

#include <kmesg.h>

typedef uint64_t handle_t;

enum class object_type {
	none,
	object_group,
	ipc_queue
};

struct base_object {
	virtual ~base_object() {};

	virtual base_object *parent() const = 0;
	virtual void set_parent(base_object *) {
		kmesg("base_object", "⚠ ERROR");
		kmesg("base_object", "");
		kmesg("base_object", "If you're seeing this, the code is in what");
		kmesg("base_object", "I thought was an unreachable state.");
		kmesg("base_object", "");
		kmesg("base_object", "I could give you advice for what to do.");
		kmesg("base_object", "But honestly, why should you trust me?");
		kmesg("base_object", "I clearly screwed this up. I'm writing a");
		kmesg("base_object", "message that should never appear, yet");
		kmesg("base_object", "I know it will probably appear someday.");
		kmesg("base_object", "");
		kmesg("base_object", "On a deep level, I know I'm not");
		kmesg("base_object", "up to this task. I'm so sorry.");
	}

	virtual object_type type() const = 0;
};

struct object_group : public base_object {
	object_group(base_object *parent = nullptr);
	~object_group() override;

	object_type type() const override {
		return object_type::object_group;
	}

	base_object *parent() const override {
		return _parent;
	}

	void set_parent(base_object *parent) override {
		_parent = parent;
	}

	handle_t add_object(base_object *obj);
	void remove_object(handle_t handle);
	base_object *get_object(handle_t handle);

	bool check_membership(uint64_t id);
	void add_member(uint64_t id);
	void remove_member(uint64_t id);

	template <typename F, typename = std::enable_if_t<!std::is_same_v<return_type_t<F>, void>>, typename ...Args>
	frg::optional<return_type_t<F>> check_membership_and(uint64_t t, F func, Args &&...args) {
		frg::unique_lock guard{_lock};
		if (!_members[t])
			return frg::null_opt;

		_dont_lock = true;
		auto result = (this->*func)(std::forward<Args...>(args...));
		_dont_lock = false;

		return result;
	}

	template <typename F, typename = std::enable_if_t<std::is_same_v<return_type_t<F>, void>>, typename ...Args>
	bool check_membership_and(uint64_t t, F func, Args &&...args) {
		frg::unique_lock guard{_lock};
		if (!_members[t])
			return false;

		_dont_lock = true;
		(this->*func)(std::forward<Args...>(args...));
		_dont_lock = false;

		return true;
	}

private:
	base_object *_parent;

	frg::hash_map<
		uint64_t,
		base_object *,
		frg::hash<uint64_t>,
		frg_allocator
	> _objects;

	frg::hash_map<
		uint64_t,
		bool,
		frg::hash<uint64_t>,
		frg_allocator
	> _members;

	std::atomic<uint64_t> _next_id;

	spinlock _lock;
	bool _dont_lock;
};

struct ipc_queue : public base_object {
	constexpr static inline size_t default_size = 128;

	ipc_queue(base_object *parent = nullptr);
	~ipc_queue() override;

	object_type type() const override {
		return object_type::ipc_queue;
	}

	void set_parent(base_object *parent) override {
		_parent = parent;
	}

	base_object *parent() const override {
		return _parent;
	}

	struct message {
		uint8_t *data;
		size_t size;

		uint64_t sender;
	};

	void send(frg::unique_ptr<message, frg_allocator> &&message);
	message &peek();
	frg::unique_ptr<message, frg_allocator> pop();

	bool has_new();
	size_t get_queue_size();

private:
	base_object *_parent;

	bool _has_new;
	size_t _n_messages;

	frg::vector<
		frg::unique_ptr<message, frg_allocator>,
		frg_allocator
	> _buffer;

	size_t _enqueue;
	size_t _dequeue;
};

#endif // OBJECT_H
