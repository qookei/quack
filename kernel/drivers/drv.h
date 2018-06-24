#ifndef DRIVER_H
#define DRIVER_h

#include <stdint.h>
#include <stddef.h>
#include "pci.h"
#include <mesg.h>
#include <interrupt/isr.h>
#include <string.h>

#define MAX_DRIVERS 8
#define MAX_DEVICES 16

#define STORAGE_DEVICE 0x1 	// hard disks, floppy drives etc
#define NET_INTERFACE 0x2	// network interfaces(rtl8139 etc)
#define INPUT_DEVICE 0x3	// keyboards, mice
#define OTHER_DEVICE 0x4	// other(ps2 controller, not kbd/mouse)

#define BUS_PCI 0x1		// on pci
#define BUS_SELF 0x2	// not on pci bus, accessed directly with ports

typedef struct pci_info {
	uint16_t vendor;
	uint16_t device;
} pci_info_t;

typedef struct device_info {

	void *type_data;
	void *bus_data;
	void *device_data;
	void *drv_info;
	uint32_t id;
	uint32_t int_no;
	
} device_info_t;

typedef struct driver_info {
	bool exists;
	
	uint32_t type;
	void *type_funcs;
	
	uint32_t bus;
	void *bus_info;
	
	uint32_t int_no;
	
	bool (*init)(device_info_t *);
	bool (*deinit)(device_info_t *);
	
	bool (*interrupt)(device_info_t *);
	
	bool (*detect)(device_info_t *);
	
	const char *name;
	
} driver_info_t;

typedef struct storage_funcs {
	bool (*write)(void *, void *, size_t, size_t);	// dev info, data, size, lba
	bool (*read)(void *, void *, size_t, size_t);	// dev info, data, size, lba
	bool (*flush)(void *);							// dev info
} storage_funcs_t;

typedef struct network_funcs {
	bool (*send)(void *, void *, size_t);	// dev info, data, size
} network_funcs_t;

typedef struct mouse_info {
	int32_t x, y;
	uint8_t btn;
} mouse_info_t;

void drv_register_devices();
void drv_detect_manual();
void list_devices();

void drv_install_pci(pci_descriptor_t *desc);

void drv_mouse_update(device_info *dev, int32_t xoff, int32_t yoff, uint8_t btn);
void drv_keyboard_update(device_info *dev, char c);

void drv_netw_recv_internal(device_info_t *dev, void *data, size_t size);

void drv_write(device_info_t *dev, void *data, size_t size, size_t lba);
void drv_read(device_info_t *dev, void *data, size_t size, size_t lba);
void drv_flush(device_info_t *dev);

bool drv_mouse_info_global(mouse_info_t *dest);
bool drv_mouse_info(device_info_t *dev, mouse_info_t *dest);

char drv_kbd_char_global();
char drv_kbd_char(device_info_t *dev);

void drv_net_send(device_info_t *dev, void *data, size_t size);
//size_t drv_net_recv(device_info_t *dev, void *data);

#endif
