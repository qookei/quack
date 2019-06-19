#ifndef DEVMGR_H
#define DEVMGR_H

#include <kobj.h>

typedef struct {
	char *name;

	size_t n_devices;
	handle_t *devices;
} devmgr_bus_t;

#define DEV_IO_REG_PORT (1 << 1)
#define DEV_IO_REG_MMIO (1 << 2)

typedef struct {
	int valid;
	uintptr_t base_addr;
	size_t len;
	int type;
} devmgr_io_reg_t;

typedef struct {
	uintptr_t base_addr;
	size_t len;
	size_t alignment;
} devmgr_dma_buf_t;

typedef struct {
	handle_t bus;

	char *name;
	uint32_t bus_vendor;
	uint32_t bus_device;
	uint32_t bus_device_type;
	uint32_t bus_device_subtype;

	char *ident;

	int interrupt;

	size_t n_io_regions;
	devmgr_io_reg_t *io_regions;

	size_t n_dma_buffers;
	devmgr_dma_buf_t *dma_buffers;
	// add more things as needed
} devmgr_dev_t;

void devmgr_init(void);
handle_t devmgr_create_bus(char *name);
handle_t devmgr_create_dev(handle_t bus, devmgr_dev_t *dev);

handle_t devmgr_get_bus_by_name(const char *name);
handle_t devmgr_get_dev_by_name(const char *name);

int devmgr_acquire_dev(handle_t hnd, int32_t this_pid);
int32_t devmgr_get_dev_owner(handle_t hnd);

size_t devmgr_get_dev_n_io_regions(handle_t hnd);
devmgr_io_reg_t *devmgr_get_dev_io_region(handle_t hnd, size_t reg);

size_t devmgr_get_dev_n_dma_buffers(handle_t hnd);
devmgr_dma_buf_t *devmgr_get_dev_dma_buffer(handle_t hnd, size_t buf);
size_t devmgr_add_dev_dma_buffer(handle_t hnd, devmgr_dma_buf_t *buf);

#endif
