#ifndef LIB_UNIQUE_H
#define LIB_UNIQUE_H

#include <type_traits>
#include <util.h>

template <typename T>
struct unique_ptr {
	unique_ptr() :_ptr{nullptr} {}
	unique_ptr(T *ptr) :_ptr{ptr} {}
	~unique_ptr() {
		if (_ptr)
			delete _ptr;
	}

	unique_ptr(const unique_ptr &) = delete;
	unique_ptr &operator=(const unique_ptr &) = delete;

	unique_ptr(unique_ptr &&p) :_ptr{p.get()} {p.reset();}
	unique_ptr &operator=(unique_ptr &&p) {_ptr = p.get(); p.reset();}

	T *get() {
		return _ptr;
	}

	T &operator*() {
		return *_ptr;
	}

	T *operator->() {
		return _ptr;
	}

	void release() {
		if (_ptr)
			delete _ptr;

		_ptr = nullptr;
	}

	bool is_set() {
		return _ptr;
	}

	void set(T *p) {
		assert(!_ptr);
		_ptr = p;
	}

private:
	void reset() {
		_ptr = nullptr;
	}

	T *_ptr;
};

// TODO: forward arguments
template <typename T, typename ...Args>
unique_ptr<T> make_unique(Args &&...args) {
	return unique_ptr<T>{new T{std::forward<Args>(args)...}};
}

#endif //LIB_UNIQUE_H
