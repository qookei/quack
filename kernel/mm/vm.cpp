#include "vm.h"
#include <arch/mm.h>
#include <util.h>
#include <kmesg.h>
#include <panic.h>

memory_mapping::memory_mapping(uintptr_t base, size_t size)
:_base{base}, _size{size}, _backing_pages(frg_allocator::get()), _list_node{},
_perms{0}, _cache_mode{0}, _mapped{false} {
	for (size_t i = 0; i < _size; i++) {
		_backing_pages.push_back({0, false, false});
	}
}

memory_mapping::~memory_mapping() {
	for (auto page : _backing_pages) {
		if (page._exists && page._allocated && page._ptr) {
			arch_mm_free_phys((void *)page._ptr, 1);
		}
	}
}

void memory_mapping::allocate_eager(int perms, int cache) {
	_perms = perms;
	_cache_mode = cache;

	for (auto &page : _backing_pages) {
		assert(!page._exists && !page._allocated);
		page._ptr = (uintptr_t)arch_mm_alloc_phys(1);
		page._exists = true;
		page._allocated = true;
	}
}

void memory_mapping::allocate_lazy(int perms, int cache) {
	_perms = perms;
	_cache_mode = cache;

	for (auto &page : _backing_pages) {
		assert(!page._exists && !page._allocated);
		page._allocated = true;
	}
}

void memory_mapping::map_to(uintptr_t address, int perms, int cache) {
	_perms = perms;
	_cache_mode = cache;

	for (auto &page : _backing_pages) {
		assert(!page._exists && !page._allocated);
		page._exists = true;
		page._allocated = false;
		page._ptr = address;
		address += vm_page_size;
	}
}

void memory_mapping::deallocate() {
	assert(!_mapped);

	for (auto &page : _backing_pages) {
		bool alloc = false;

		if (page._allocated) {
			page._allocated = false;
			alloc = true;
		}

		if (page._exists) {
			if (alloc)
				arch_mm_free_phys((void *)page._ptr, 1);
			page._exists = false;
		}

		page._ptr = 0;
	}
}

vm_fault_result memory_mapping::fault_hit(uintptr_t address) {
	if (address < _base || address > (_base + _size * vm_page_size))
		panic(NULL, "memory_mapping::fault_hit(0x%lx) called outside of mapping (0x%lx-0x%lx)",
			address, _base, _base + _size * vm_page_size);

	uintptr_t idx = (address - _base) / vm_page_size;
	auto &page = _backing_pages[idx];

	if (page._allocated && !page._exists) {
		// hit overcommited page
		if (touch(idx))
			return vm_fault_result::remap_valid;
		else
			return vm_fault_result::ignore;
	} else if (page._exists) {
		// invalid access
		return vm_fault_result::invalid_access;
	} else if (!page._allocated && !page._exists) {
		// not mapped
		return vm_fault_result::not_mapped;
	}

	return vm_fault_result::invalid;
}

bool memory_mapping::touch(size_t idx) {
	auto &page = _backing_pages[idx];

	if (!page._exists) {
		assert (page._allocated);
		uintptr_t ptr = (uintptr_t)arch_mm_alloc_phys(1);
		if (!ptr) {
			return false;
		}

		page._exists = true;
		page._ptr = ptr;
		return true;
	}

	return false;
}

bool memory_mapping::load(ptrdiff_t offset, void *data, size_t size) {
	assert(offset >= 0);

	size_t pages = (size + vm_page_size - 1) / vm_page_size;
	size_t start = offset / vm_page_size;

	if (((offset + size + vm_page_size - 1) / vm_page_size) > _size)
		return false;

	bool touched = false;

	for (size_t i = start; i < start + pages; i++) {
		touched = touched || touch(i);
	}

	arch_mm_mapping_load(this, offset, data, size);

	return touched;
}

bool memory_mapping::store(ptrdiff_t offset, void *data, size_t size) {
	assert(offset >= 0);

	size_t pages = (size + vm_page_size - 1) / vm_page_size;
	size_t start = offset / vm_page_size;

	if (((offset + size + vm_page_size - 1) / vm_page_size) > _size)
		return false;

	bool touched = false;

	for (size_t i = start; i < start + pages; i++) {
		touched = touched || touch(i);
	}

	arch_mm_mapping_store(this, offset, data, size);

	return touched;
}

address_space::address_space() {
	_vmm_ctx = arch_mm_create_context();

	memory_hole *hole = new memory_hole{ vm_user_base,
		(vm_user_end - vm_user_base) / vm_page_size, {}};

	_memory_holes.push_back(hole);
}

address_space::~address_space() {
	arch_mm_destroy_context(_vmm_ctx);

	for (auto hole : _memory_holes) {
		_memory_holes.erase(hole);
		delete hole;
	}

	for (auto region : _mapped_regions) {
		_mapped_regions.erase(region);
		delete region;
	}

}

memory_hole *address_space::find_free_hole(size_t size) {
	for (auto hole : _memory_holes) {
		if (hole->_size >= size)
			return hole;
	}

	return nullptr;
}

memory_mapping *address_space::bind_hole(memory_hole *hole, size_t size) {
	assert(hole->_size >= size);

	uintptr_t base = hole->_base;

	if (hole->_size == size) {
		_memory_holes.erase(hole);
		delete hole;

		memory_mapping *mapping = new memory_mapping{base, size};
		memory_mapping *succ = find_succ_mapping(mapping);

		if (succ) {
			_mapped_regions.insert(succ, mapping);
		} else {
			_mapped_regions.push_back(mapping);
		}

		return mapping;
	} else {
		hole->_base += size * vm_page_size;
		hole->_size -= size;

		memory_mapping *mapping = new memory_mapping{base, size};
		memory_mapping *succ = find_succ_mapping(mapping);

		if (succ) {
			_mapped_regions.insert(succ, mapping);
		} else {
			_mapped_regions.push_back(mapping);
		}

		return mapping;
	}
}

memory_mapping *address_space::bind_exact(uintptr_t base, size_t size) {
	assert(size);
	assert(base);

	// ensure there are no split holes
	merge_holes();

	memory_hole *target_hole = nullptr;
	for (auto hole : _memory_holes) {
		if (hole->_base <= base
				&& (hole->_base + hole->_size * vm_page_size)
				>= (base + size * vm_page_size)) {
			target_hole = hole;
			break;
		}
	}

	if (!target_hole) {
		kmesg("vm", "failed to bind exact (base=%016lx, size=%lu)", base, size);
		return nullptr;
	}

	if (base == target_hole->_base && size == target_hole->_size) {
		// exact match
		_memory_holes.erase(target_hole);
		delete target_hole;
	} else if (base == target_hole->_base) {
		// leading
		target_hole->_base += size * vm_page_size;
		target_hole->_size -= size;
	} else if (base + size * vm_page_size == target_hole->_base + target_hole->_size * vm_page_size) {
		// trailing
		target_hole->_size -= size;
	} else {
		// in the middle
		size_t old_base = target_hole->_base;
		memory_hole *new_hole = new memory_hole{target_hole->_base, (base - target_hole->_base) / vm_page_size, {}};
		target_hole->_base = base + size * vm_page_size;
		target_hole->_size = target_hole->_size - size - (base - old_base) / vm_page_size;
		_memory_holes.insert(target_hole, new_hole);
	}

	memory_mapping *mapping = new memory_mapping{base, size};
	memory_mapping *succ = find_succ_mapping(mapping);

	if (succ) {
		_mapped_regions.insert(succ, mapping);
	} else {
		_mapped_regions.push_back(mapping);
	}

	return mapping;
}

void address_space::unbind_region(memory_mapping *region) {
	memory_hole *hole = new memory_hole{region->_base, region->_size, {}};
	memory_hole *succ = find_succ_hole(hole);

	if (succ) {
		_memory_holes.insert(succ, hole);
	} else {
		_memory_holes.push_front(hole);
	}

	_mapped_regions.erase(region);

	merge_holes();
}

void address_space::map_region(memory_mapping *region) {
	for (size_t i = 0; i < region->_size; i++) {
		if (region->_backing_pages[i]._exists) {
			arch_mm_map(_vmm_ctx,
				(void *)(region->_base + i * vm_page_size),
				(void *)region->_backing_pages[i]._ptr, 1,
				region->_perms, region->_cache_mode);
		}
	}

	region->_mapped = true;
}

void address_space::unmap_region(memory_mapping *region) {
	assert(region->_mapped);

	for (size_t i = 0; i < region->_size; i++) {
		if (region->_backing_pages[i]._exists) {
			arch_mm_unmap(_vmm_ctx,
				(void *)(region->_base + i * vm_page_size), 1);
		}
	}

	region->_mapped = false;
}

uintptr_t address_space::allocate_eager(size_t size, int perms, int cache) {
	auto hole = find_free_hole(size);
	if (!hole)
		return 0;

	auto mem = bind_hole(hole, size);
	if (!mem)
		return 0;

	mem->allocate_eager(perms, cache);
	map_region(mem);

	return mem->_base;
}

uintptr_t address_space::allocate_lazy(size_t size, int perms, int cache) {
	auto hole = find_free_hole(size);
	if (!hole)
		return 0;

	auto mem = bind_hole(hole, size);
	if (!mem)
		return 0;

	mem->allocate_lazy(perms, cache);
	map_region(mem);

	return mem->_base;
}

uintptr_t address_space::map(uintptr_t address, size_t size, int perms, int cache) {
	auto hole = find_free_hole(size);
	if (!hole)
		return 0;

	auto mem = bind_hole(hole, size);
	if (!mem)
		return 0;

	mem->map_to(address, perms, cache);
	map_region(mem);

	return mem->_base;
}

uintptr_t address_space::allocate_exact_eager(uintptr_t base, size_t size, int perms, int cache) {
	auto mem = bind_exact(base, size);
	if (!mem)
		return 0;

	mem->allocate_eager(perms, cache);
	map_region(mem);

	return mem->_base;
}

uintptr_t address_space::allocate_exact_lazy(uintptr_t base, size_t size, int perms, int cache) {
	auto mem = bind_exact(base, size);
	if (!mem)
		return 0;

	mem->allocate_lazy(perms, cache);
	map_region(mem);

	return mem->_base;
}

uintptr_t address_space::map_exact(uintptr_t base, uintptr_t address, size_t size, int perms, int cache) {
	auto mem = bind_exact(base, size);
	if (!mem)
		return 0;

	mem->map_to(address, perms, cache);
	map_region(mem);

	return mem->_base;
}

void address_space::destroy(memory_mapping *region) {
	unmap_region(region);
	unbind_region(region);
	delete region;
}

void address_space::destroy(uintptr_t address) {
	destroy(region_for_address(address));
}

memory_mapping *address_space::region_for_address(uintptr_t address) {
	for (auto region : _mapped_regions) {
		if (address >= region->_base &&
				address <= (region->_base + region->_size * vm_page_size))
			return region;
	}

	return nullptr;
}

bool address_space::fault_hit(uintptr_t address) {
	auto region = region_for_address(address);

	if (!region)
		return false;

	kmesg("vm", "fault in region %p, address %lx", region, address);

	auto result = region->fault_hit(address);

	switch (result) {
		case vm_fault_result::invalid:
		case vm_fault_result::invalid_access:
		case vm_fault_result::not_mapped:
			return false;

		case vm_fault_result::remap_valid:
			map_region(region);
			[[fallthrough]];
		case vm_fault_result::ignore:
			return true;
	}

	return false;
}

void address_space::touch(uintptr_t address) {
	auto region = region_for_address(address);

	if (!region)
		return;

	region->touch((address - region->_base )/ vm_page_size);
}

memory_hole *address_space::find_pre_hole(memory_hole *in) {
	memory_hole *candidate = *_memory_holes.begin();
	size_t n_candidates = 0;

	for (auto hole : _memory_holes) {
		if ((hole->_base >= candidate->_base)
				&& (hole->_base + hole->_size * vm_page_size <
					in->_base + in->_size * vm_page_size)
				&& (hole != in)) {
			n_candidates++;
			candidate = hole;
		}
	}

	return n_candidates ? candidate : nullptr;
}

memory_mapping *address_space::find_pre_mapping(memory_mapping *in) {
	memory_mapping *candidate = *_mapped_regions.begin();
	size_t n_candidates = 0;

	for (auto region : _mapped_regions) {
		if ((region->_base >= candidate->_base)
				&& (region->_base + region->_size * vm_page_size <
					in->_base + in->_size * vm_page_size)
				&& (region != in)) {
			n_candidates++;
			candidate = region;
		}
	}

	return n_candidates ? candidate : nullptr;
}

template <typename T>
ptrdiff_t distance_between(T a, T b) {
	return (a->_base + a->_size * vm_page_size) - b->_base;
}

memory_hole *address_space::find_succ_hole(memory_hole *in) {
	memory_hole *candidate = *_memory_holes.begin();
	bool has_candidate = false;
	ptrdiff_t best_distance = 0;

	for (auto region : _memory_holes) {
		if (region->_base < in->_base)
			continue;

		if (!has_candidate) {
			has_candidate = true;
			candidate = region;
			best_distance = distance_between(region, in);
		} else if (ptrdiff_t d = distance_between(region, in); d < best_distance) {
			candidate = region;
			best_distance = d;
		}
	}

	return has_candidate ? candidate : nullptr;
}

memory_mapping *address_space::find_succ_mapping(memory_mapping *in) {
	memory_mapping *candidate = *_mapped_regions.begin();
	bool has_candidate = false;
	ptrdiff_t best_distance = 0;

	for (auto region : _mapped_regions) {
		if (region->_base < in->_base)
			continue;

		if (!has_candidate) {
			has_candidate = true;
			candidate = region;
			best_distance = distance_between(region, in);
		} else if (ptrdiff_t d = distance_between(region, in); d < best_distance) {
			candidate = region;
			best_distance = d;
		}
	}

	return has_candidate ? candidate : nullptr;
}

void address_space::merge_holes() {
//	kmesg("vm", "holes before merge:");
//	for (auto hole : _memory_holes) {
//		kmesg("vm", "\t%016lx-%016lx", hole->_base, hole->_base + hole->_size * vm_page_size);
//	}

	for (auto hole : _memory_holes) {
		auto prev = hole->_list_node.previous,
			next = hole->_list_node.next;

		if (prev && !distance_between(prev, hole)) {
			/* merge */
			hole->_size += prev->_size;
			hole->_base = prev->_base;

			_memory_holes.erase(prev);
			delete prev;
		}

		if (next && !distance_between(hole, next)) {
			/* merge */
			hole->_size += next->_size;

			_memory_holes.erase(next);
			delete next;
		}
	}

//	kmesg("vm", "holes after merge:");
//	for (auto hole : _memory_holes) {
//		kmesg("vm", "\t%016lx-%016lx", hole->_base, hole->_base + hole->_size * vm_page_size);
//	}
}

void address_space::load(uintptr_t address, void *data, size_t size) {
	auto region = region_for_address(address);

	if (!region)
		return;

	if (region->load(address - region->_base, data, size)) {
		map_region(region);
	}
}

void address_space::store(uintptr_t address, void *data, size_t size) {
	auto region = region_for_address(address);

	if (!region)
		return;

	if (region->store(address - region->_base, data, size)) {
		map_region(region);
	}
}

void address_space::debug() {
	kmesg("vm", "-------- this = %016p --------", this);
	kmesg("vm", "mapped regions:");
	for (auto region : _mapped_regions) {
		kmesg("vm", "\t%016lx-%016lx", region->_base, region->_base + region->_size * vm_page_size);
	}

	kmesg("vm", "memory holes:");
	for (auto hole : _memory_holes) {
		kmesg("vm", "\t%016lx-%016lx", hole->_base, hole->_base + hole->_size * vm_page_size);
	}
	kmesg("vm", "-----------------------------------------");
}

void *address_space::vmm_ctx() {
	return _vmm_ctx;
}
