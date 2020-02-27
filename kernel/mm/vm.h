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

// TODO: move into arch/mm.h
namespace vm_perm {
	constexpr inline int read = 1;
	constexpr inline int write = 2;
	constexpr inline int execute = 4;
	constexpr inline int user = 8;

	constexpr inline int ro = read;
	constexpr inline int rw = read | write;
	constexpr inline int rx = read | execute;
	constexpr inline int rwx = read | write | execute;

	constexpr inline int uro = user | read;
	constexpr inline int urw = user | read | write;
	constexpr inline int urx = user | read | execute;
	constexpr inline int urwx = user | read | write | execute;
}

// TODO: move into arch/mm.h
namespace vm_cache_mode {
	constexpr inline int wb = 0;
	constexpr inline int wt = 1;
	constexpr inline int wc = 2;
	constexpr inline int wp = 3;
	constexpr inline int uc = 4;
}

struct memory_mapping {
	memory_mapping(uintptr_t base, size_t size);
	~memory_mapping();

	void allocate_eager(int perms, int cache);
	void allocate_lazy(int perms, int cache);
	void map_to(uintptr_t address, int perms, int cache);
	void deallocate();

	vm_fault_result fault_hit(uintptr_t address);

	uintptr_t _base;
	size_t _size;

	frg::vector<backing_page, frg_allocator> _backing_pages;

	frg::default_list_hook<memory_mapping> _list_node;

	int _perms;
	int _cache_mode;

	bool _mapped;
};

struct memory_hole {
	uintptr_t _base;
	size_t _size;

	frg::default_list_hook<memory_hole> _list_node;
};

// TODO: implement {load,store}_foreign to perform transactions across address spaces
// TODO: test this properly
// TODO: implement destruction

struct address_space {
	address_space();

	void create();

	memory_hole *find_free_hole(size_t size);
	memory_mapping *bind_hole(memory_hole *hole, size_t size);
	memory_mapping *bind_exact(uintptr_t base, size_t size);
	void unbind_region(memory_mapping *region);

	void map_region(memory_mapping *region);
	void unmap_region(memory_mapping *region);

	uintptr_t allocate_eager(size_t size, int perms, int cache = vm_cache_mode::wb);
	uintptr_t allocate_lazy(size_t size, int perms, int cache = vm_cache_mode::wb);
	uintptr_t map(uintptr_t address, size_t size, int perms, int cache = vm_cache_mode::wb);

	uintptr_t allocate_exact_eager(uintptr_t base, size_t size, int perms, int cache = vm_cache_mode::wb);
	uintptr_t allocate_exact_lazy(uintptr_t base, size_t size, int perms, int cache = vm_cache_mode::wb);
	uintptr_t map_exact(uintptr_t base, uintptr_t address, size_t size, int perms, int cache = vm_cache_mode::wb);

	void destroy(memory_mapping *region);
	void destroy(uintptr_t address);

	memory_mapping *region_for_address(uintptr_t address);

	bool fault_hit(uintptr_t address);

	void debug();

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

	void merge_holes();

	void *_vmm_ctx;
};

#endif
