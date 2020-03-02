#ifndef MM_VM_H
#define MM_VM_H

#include <lib/frg_allocator.h>
#include <new>
#include <frg/list.hpp>
#include <frg/vector.hpp>
#include <frg/intrusive.hpp>

#include <stdint.h>
#include <stddef.h>

#include <atomic>

#include <arch/mm.h>

struct backing_page {
	uintptr_t _ptr;

	bool _exists;
	bool _allocated;
};

enum class vm_fault_result {
	invalid,
	remap_valid,
	ignore,
	invalid_access,
	not_mapped
};

struct memory_mapping {
	memory_mapping(uintptr_t base, size_t size);
	~memory_mapping();

	memory_mapping(const memory_mapping &) = delete;
	memory_mapping &operator=(const memory_mapping &) = delete;

	friend void swap(memory_mapping &a, memory_mapping &b) {
		using std::swap;
		swap(a._backing_pages, b._backing_pages);
		swap(a._base, b._base);
		swap(a._size, b._size);
		swap(a._mapped, b._mapped);
		swap(a._perms, b._perms);
		swap(a._cache_mode, b._cache_mode);
		swap(a._list_node, b._list_node);
	}

	memory_mapping(memory_mapping &&other) {
		swap(*this, other);
	}

	memory_mapping &operator=(memory_mapping &&other) {
		swap(*this, other);
		return *this;
	}

	void allocate(int perms, int cache);
	void map_to(uintptr_t address, int perms, int cache);
	void deallocate();

	vm_fault_result fault_hit(uintptr_t address);
	bool touch(size_t idx);
	bool touch_all();

	bool load(ptrdiff_t offset, void *data, size_t size);
	bool store(ptrdiff_t offset, void *data, size_t size);

	uintptr_t _base;
	size_t _size;

	frg::vector<backing_page, frg_allocator> _backing_pages;

	frg::default_list_hook<memory_mapping> _list_node;

	int _perms;
	int _cache_mode;

	bool _mapped;

private:
	bool ensure_load_store(ptrdiff_t offset, size_t size);
};

struct memory_hole {
	uintptr_t _base;
	size_t _size;

	frg::default_list_hook<memory_hole> _list_node;
};

struct address_space {
	address_space();
	~address_space();

	friend void swap(address_space &a, address_space &b) {
		using std::swap;
		swap(a._mapped_regions, b._mapped_regions);
		swap(a._memory_holes, b._memory_holes);
		swap(a._vmm_ctx, b._vmm_ctx);
	}

	address_space(const address_space &) = delete;
	address_space &operator=(const address_space &) = delete;

	address_space(address_space &&other) {
		swap(*this, other);
	}

	address_space &operator=(address_space &&other) {
		swap(*this, other);
		return *this;
	}
	memory_hole *find_free_hole(size_t size);
	memory_mapping *bind_hole(memory_hole *hole, size_t size);
	memory_mapping *bind_exact(uintptr_t base, size_t size);
	void unbind_region(memory_mapping *region);

	void map_region(memory_mapping *region);
	void unmap_region(memory_mapping *region);

	uintptr_t allocate(size_t size, int perms, int cache = vm_cache::wb);
	uintptr_t map(uintptr_t address, size_t size, int perms, int cache = vm_cache::wb);

	uintptr_t allocate_at(uintptr_t base, size_t size, int perms, int cache = vm_cache::wb);
	uintptr_t map_at(uintptr_t base, uintptr_t address, size_t size, int perms, int cache = vm_cache::wb);

	void destroy(memory_mapping *region);
	void destroy(uintptr_t address);

	memory_mapping *region_for_address(uintptr_t address);

	bool fault_hit(uintptr_t address);
	void touch(uintptr_t address);
	void touch_all(uintptr_t address);

	void load(uintptr_t address, void *data, size_t size);
	void store(uintptr_t address, void *data, size_t size);

	void debug();

	void *vmm_ctx();
private:
	frg::intrusive_list<
		memory_mapping,
		frg::locate_member<
			memory_mapping,
			frg::default_list_hook<memory_mapping>,
			&memory_mapping::_list_node
		>
	> _mapped_regions;

	frg::intrusive_list<
		memory_hole,
		frg::locate_member<
			memory_hole,
			frg::default_list_hook<memory_hole>,
			&memory_hole::_list_node
		>
	> _memory_holes;

	memory_hole *find_pre_hole(memory_hole *in);
	memory_mapping *find_pre_mapping(memory_mapping *in);

	memory_hole *find_succ_hole(memory_hole *in);
	memory_mapping *find_succ_mapping(memory_mapping *in);

	void put(memory_mapping *mapping);
	void put(memory_hole *hole);

	void merge_holes();

	void *_vmm_ctx;
};

#endif
