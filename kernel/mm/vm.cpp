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
		address += ARCH_MM_PAGE_SIZE;
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
	if (address < _base || address > (_base + _size * ARCH_MM_PAGE_SIZE))
		panic(NULL, "memory_mapping::fault_hit(0x%lx) called outside of mapping (0x%lx-0x%lx)",
			address, _base, _base + _size * ARCH_MM_PAGE_SIZE);

	uintptr_t idx = (address - _base) / ARCH_MM_PAGE_SIZE;
	auto &page = _backing_pages[idx];

	if (page._allocated && !page._exists) {
		// hit overcommited page
		uintptr_t ptr = (uintptr_t)arch_mm_alloc_phys(1);
		if (!ptr) {
			return vm_fault_result::ignore;
		}

		page._exists = true;
		page._ptr = ptr;
		return vm_fault_result::remap_valid;
	} else if (page._exists) {
		// invalid access
		return vm_fault_result::invalid_access;
	} else if (!page._allocated && !page._exists) {
		// not mapped
		return vm_fault_result::not_mapped;
	}

	return vm_fault_result::invalid;
}

address_space::address_space() {}

// TODO: move to arch/mm.h
#define MEMORY_BASE 0x1000LLU
#define MEMORY_END 0x800000000000LLU

void address_space::create() {
	_vmm_ctx = arch_mm_create_context();

	memory_hole *hole = new memory_hole{MEMORY_BASE, (MEMORY_END - MEMORY_BASE) / ARCH_MM_PAGE_SIZE, {}};

	_memory_holes.push_back(hole);
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
		hole->_base += size * ARCH_MM_PAGE_SIZE;
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
				&& (hole->_base + hole->_size * ARCH_MM_PAGE_SIZE)
				>= (base + size * ARCH_MM_PAGE_SIZE)) {
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
		target_hole->_base += size * ARCH_MM_PAGE_SIZE;
		target_hole->_size -= size;
	} else if (base + size * ARCH_MM_PAGE_SIZE == target_hole->_base + target_hole->_size * ARCH_MM_PAGE_SIZE) {
		// trailing
		target_hole->_size -= size;
	} else {
		// in the middle
		size_t old_base = target_hole->_base;
		memory_hole *new_hole = new memory_hole{target_hole->_base, (base - target_hole->_base) / ARCH_MM_PAGE_SIZE, {}};
		target_hole->_base = base + size * ARCH_MM_PAGE_SIZE;
		target_hole->_size = target_hole->_size - size - (base - old_base) / ARCH_MM_PAGE_SIZE;
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
				(void *)(region->_base + i * ARCH_MM_PAGE_SIZE),
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
				(void *)(region->_base + i * ARCH_MM_PAGE_SIZE), 1);
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
		if (region->_base >= address &&
				address <= (region->_base + region->_size * ARCH_MM_PAGE_SIZE))
			return region;
	}

	return nullptr;
}

bool address_space::fault_hit(uintptr_t address) {
	auto region = region_for_address(address);

	if (!region)
		return false;

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

memory_hole *address_space::find_pre_hole(memory_hole *in) {
	memory_hole *candidate = *_memory_holes.begin();
	size_t n_candidates = 0;

	for (auto hole : _memory_holes) {
		if ((hole->_base >= candidate->_base)
				&& (hole->_base + hole->_size * ARCH_MM_PAGE_SIZE <
					in->_base + in->_size * ARCH_MM_PAGE_SIZE)
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
				&& (region->_base + region->_size * ARCH_MM_PAGE_SIZE <
					in->_base + in->_size * ARCH_MM_PAGE_SIZE)
				&& (region != in)) {
			n_candidates++;
			candidate = region;
		}
	}

	return n_candidates ? candidate : nullptr;
}

template <typename T>
ptrdiff_t distance_between(T a, T b) {
	return (a->_base + a->_size * ARCH_MM_PAGE_SIZE) - b->_base;
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
//		kmesg("vm", "\t%016lx-%016lx", hole->_base, hole->_base + hole->_size * ARCH_MM_PAGE_SIZE);
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
//		kmesg("vm", "\t%016lx-%016lx", hole->_base, hole->_base + hole->_size * ARCH_MM_PAGE_SIZE);
//	}
}

void address_space::debug() {
	kmesg("vm", "-------- this = %016p --------", this);
	kmesg("vm", "mapped regions:");
	for (auto region : _mapped_regions) {
		kmesg("vm", "\t%016lx-%016lx", region->_base, region->_base + region->_size * ARCH_MM_PAGE_SIZE);
	}

	kmesg("vm", "memory holes:");
	for (auto hole : _memory_holes) {
		kmesg("vm", "\t%016lx-%016lx", hole->_base, hole->_base + hole->_size * ARCH_MM_PAGE_SIZE);
	}
	kmesg("vm", "-----------------------------------------");
}
