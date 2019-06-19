#include "devmgr.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mm/heap.h>
#include <util.h>
#include <kmesg.h>

static handle_t *devices = NULL;
static size_t n_devices = 0;

static handle_t *busses = NULL;
static size_t n_busses = 0;

void devmgr_init(void) {
}

static void append_to_bus(handle_t hnd, handle_t dev) {
	kobj_t *obj = kobj_get(hnd);
	devmgr_bus_t *bus = obj->data;

	bus->n_devices++;
	bus->devices = krealloc(bus->devices, bus->n_devices * sizeof(handle_t));
	bus->devices[bus->n_devices - 1] = dev;
}

handle_t devmgr_create_bus(char *name) {
	n_busses++;
	busses = krealloc(busses, n_busses * sizeof(handle_t));

	handle_t hnd = kobj_create_new(0, name, 0, sizeof(devmgr_bus_t));
	assert(hnd && "failed to allocate a handle");
	busses[n_busses - 1] = hnd;

	kobj_t *obj = kobj_get(hnd);
	devmgr_bus_t *bus = obj->data;
	bus->name = kmalloc(strlen(name) + 1);
	strcpy(bus->name, name);
	bus->n_devices = 0;
	bus->devices = NULL;

	return hnd;
}

handle_t devmgr_create_dev(handle_t bus, devmgr_dev_t *dev) {
	assert(kobj_get(bus) && "invalid bus");

	n_devices++;
	devices = krealloc(devices, n_devices * sizeof(handle_t));

	handle_t hnd = kobj_create_new(0, dev->name, 0, sizeof(devmgr_dev_t));
	assert(hnd && "failed to allocate a handle");
	devices[n_devices - 1] = hnd;

	kobj_t *obj = kobj_get(hnd);
	devmgr_dev_t *d = obj->data;

	d->name = kmalloc(strlen(dev->name) + 1);
	strcpy(d->name, dev->name);
	d->ident = kmalloc(strlen(dev->ident) + 1);
	strcpy(d->ident, dev->ident);

	d->bus = bus;
	d->bus_vendor = dev->bus_vendor;
	d->bus_device = dev->bus_device;
	d->bus_device_type = dev->bus_device_type;
	d->bus_device_subtype = dev->bus_device_subtype;

	d->n_io_regions = dev->n_io_regions;
	d->io_regions = kcalloc(sizeof(devmgr_io_reg_t), d->n_io_regions);
	memcpy(d->io_regions, dev->io_regions, sizeof(devmgr_io_reg_t) * d->n_io_regions);

	d->n_dma_buffers = 0;
	d->dma_buffers = NULL;

	append_to_bus(bus, hnd);

	return hnd;
}

handle_t devmgr_get_bus_by_name(const char *name) {
	for (size_t i = 0; i < n_busses; i++) {
		kobj_t *obj = kobj_get(busses[i]);
		assert(obj && "missing bus object which should exist");
		devmgr_bus_t *bus = obj->data;
		if (!strcmp(name, bus->name)) return busses[i];
	}

	kmesg("devmgr", "failed to find bus '%s'", name);

	return 0;
}

handle_t devmgr_get_dev_by_name(const char *name) {
	for (size_t i = 0; i < n_devices; i++) {
		kobj_t *obj = kobj_get(devices[i]);
		assert(obj && "missing device object which should exist");
		devmgr_dev_t *dev = obj->data;
		if (!strcmp(name, dev->name)) return devices[i];
	}

	kmesg("devmgr", "failed to find device '%s'", name);

	return 0;
}

int devmgr_acquire_dev(handle_t hnd, int32_t this_pid) {
	kobj_t *obj = kobj_get(hnd);
	if (!obj->owner_pid) {
		obj->owner_pid = this_pid;
		return 1;
	}

	return 0;
}

int32_t devmgr_get_dev_owner(handle_t hnd) {
	kobj_t *obj = kobj_get(hnd);
	return obj->owner_pid;
}

size_t devmgr_get_dev_n_io_regions(handle_t hnd) {
	kobj_t *obj = kobj_get(hnd);
	assert(obj && "invalid handle");
	return ((devmgr_dev_t *)obj->data)->n_io_regions;
}

devmgr_io_reg_t *devmgr_get_dev_io_region(handle_t hnd, size_t reg) {
	if (reg >= devmgr_get_dev_n_io_regions(hnd))
		return NULL;

	kobj_t *obj = kobj_get(hnd);
	assert(obj && "invalid handle");
	return &((devmgr_dev_t *)obj->data)->io_regions[reg];
}

size_t devmgr_get_dev_n_dma_buffers(handle_t hnd) {
	kobj_t *obj = kobj_get(hnd);
	assert(obj && "invalid handle");
	return ((devmgr_dev_t *)obj->data)->n_dma_buffers;
}

devmgr_dma_buf_t *devmgr_get_dev_dma_buffer(handle_t hnd, size_t buf) {
	if (buf >= devmgr_get_dev_n_dma_buffers(hnd))
		return NULL;

	kobj_t *obj = kobj_get(hnd);
	assert(obj && "invalid handle");
	return &((devmgr_dev_t *)obj->data)->dma_buffers[buf];
}

size_t devmgr_add_dev_dma_buffer(handle_t hnd, devmgr_dma_buf_t *buf) {
	kobj_t *obj = kobj_get(hnd);
	assert(obj && "invalid handle");
	devmgr_dev_t *dev = obj->data;

	dev->n_dma_buffers++;
	dev->dma_buffers = krealloc(dev->dma_buffers, dev->n_dma_buffers * sizeof(devmgr_dma_buf_t));
	memcpy(&dev->dma_buffers[dev->n_dma_buffers - 1], buf, sizeof(devmgr_dma_buf_t));

	return dev->n_dma_buffers - 1;
}

void devmgr_dump_devices(void) {
	kmesg("devmgr", "devices present:");
	for (size_t i = 0; i < n_busses; i++) {
		devmgr_bus_t *bus = kobj_get(busses[i])->data;
		kmesg("devmgr", "'%s' (%lu devices):", bus->name, bus->n_devices);
		for (size_t j = 0; j < bus->n_devices; j++) {
			kobj_t *obj = kobj_get(devices[bus->devices[j]]);
			assert(obj && "handle doesn't exist while it should");
			devmgr_dev_t *dev = obj->data;
			kmesg("devmgr", "\t'%s': %s owned by %d%s, has %lu io regions", dev->name, dev->ident, obj->owner_pid, obj->owner_pid ? "" : "(no one)", dev->n_io_regions);
		}
	}
}
