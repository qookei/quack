#include "drv.h"
#include <kheap/heap.h>

#ifdef DRIVER_DUMMY1
driver_info_t *dummy1_info();
#endif

#ifdef DRIVER_PS2M
driver_info_t *ps2m_info();
#endif

#ifdef DRIVER_PS2K
driver_info_t *ps2k_info();
#endif

#define KBUFFER 256

typedef struct input_device_data {
	int32_t mouse_x, mouse_y;
	uint8_t mouse_btn;
	bool mouse_changed;
	
	char buffer[KBUFFER];
	uint32_t index;
} input_device_data_t;

driver_info_t **drvs;
device_info_t **drv_devices;
size_t n_drvs = 0;
uint32_t glob_id = 0;

void drv_add_to_drvs(driver_info_t *drv) {
	if (!drvs) {
		drvs = (driver_info_t **)kmalloc(sizeof(driver_info_t *) * MAX_DRIVERS);
		memset(drvs, 0, sizeof(driver_info_t *) * MAX_DRIVERS);
	}
	
	drvs[n_drvs] = drv;
	n_drvs++;
}

bool drv_int_handler(interrupt_cpu_state *state) {
	for (uint32_t i = 0; i < MAX_DEVICES; i++) {
		if (!drv_devices[i]) continue;
		if (state->interrupt_number == drv_devices[i]->int_no) {
			if (((driver_info_t *)(drv_devices[i]->drv_info))->interrupt(drv_devices[i]))
				break;
		}
	}
	return true;
}

void drv_register_devices() {
	
	early_mesg(LEVEL_DBG, "drv", "register drivers");
	
	drv_devices = (device_info_t **)kmalloc(sizeof(device_info_t *) * MAX_DEVICES);
	memset(drv_devices, 0, sizeof(device_info_t *) * MAX_DEVICES);
	
	#ifdef DRIVER_DUMMY1
	drv_add_to_drvs(dummy1_info());
	#endif
	
	#ifdef DRIVER_PS2M
	drv_add_to_drvs(ps2m_info());
	#endif
	
}

uint32_t find_free_device() {
	uint32_t i = 0;
	while(i < MAX_DEVICES && drv_devices[i]) i++;
	return i;
}

void drv_detect_manual_one(driver_info_t *drv) {

	uint32_t dev = find_free_device();
	if (dev == MAX_DEVICES) {
		early_mesg(LEVEL_ERR, "drv", "no free device slot found!");
	}
	
	drv_devices[dev] = (device_info_t *)kmalloc(sizeof(device_info_t));
	drv_devices[dev]->drv_info = drv;
	drv_devices[dev]->device_data = NULL;
	drv_devices[dev]->id = glob_id++;
	drv_devices[dev]->int_no = drv->int_no;
	drv_devices[dev]->type_data = kmalloc(sizeof(input_device_data_t));
	drv_devices[dev]->bus_data = NULL;
	
	memset(drv_devices[dev]->type_data, 0, sizeof(input_device_data_t));
	
	bool b = ((driver_info_t *)(drv_devices[dev]->drv_info))->detect(drv_devices[dev]);
	if(!b || !((driver_info_t *)(drv_devices[dev]->drv_info))->init(drv_devices[dev])) {
		// remove device
		kfree(drv_devices[dev]);
		drv_devices[dev] = NULL;
	} else {
		if (drv_devices[dev]->int_no)
			register_interrupt_handler(drv_devices[dev]->int_no, drv_int_handler);
	}
}

void drv_detect_manual() {
	early_mesg(LEVEL_DBG, "drv", "manual device detect");
	
	#ifdef DRIVER_PS2M
	drv_detect_manual_one(ps2m_info());
	#endif
	
	#ifdef DRIVER_PS2K
	drv_detect_manual_one(ps2k_info());
	#endif
}


uint32_t find_driver_for_pci(pci_info_t *info) {
	for (uint32_t i = 0; i < n_drvs; i++) {
		if (!drvs[i] || !drvs[i]->exists || drvs[i]->bus != BUS_PCI) continue;
		if (!memcmp(info, drvs[i]->bus_info, sizeof(pci_info_t)))
			return i;
	}
	return 0xFFFFFFFF;
}

void list_devices() {
	for (uint32_t i = 0; i < MAX_DEVICES; i++) {
		if (!drv_devices[i]) continue;
		early_mesg(LEVEL_INFO, "drv", "device using driver %s irq %x id %u", ((driver_info_t *)(drv_devices[i]->drv_info))->name, drv_devices[i]->int_no, drv_devices[i]->id);
	}
}

void drv_install_pci(pci_descriptor_t *desc) {
	
	pci_info_t f;
	f.vendor = desc->vendor_id;
	f.device = desc->device_id;
	uint32_t driver = find_driver_for_pci(&f);
	
	if (driver != 0xFFFFFFFF) {
		
		early_mesg(LEVEL_INFO, "drv", "driver found for %04x:%04x!", f.vendor, f.device);
		uint32_t dev = find_free_device();
		if (dev == MAX_DEVICES) {
			early_mesg(LEVEL_ERR, "drv", "no free device slot found!");
		}
		
		drv_devices[dev] = (device_info_t *)kmalloc(sizeof(device_info_t));
		drv_devices[dev]->bus_data = kmalloc(sizeof(pci_descriptor_t));
		drv_devices[dev]->drv_info = drvs[driver];
		drv_devices[dev]->device_data = NULL;
		drv_devices[dev]->id = glob_id++;
		drv_devices[dev]->type_data = NULL;
		
		memcpy(drv_devices[dev]->bus_data, desc, sizeof(pci_descriptor_t));
		
		if (drvs[driver]->int_no) {
			drv_devices[dev]->int_no = drvs[driver]->int_no;
		} else if (desc->interrupt) {
			drv_devices[dev]->int_no = desc->interrupt + 0x20;
		} else {
			drv_devices[dev]->int_no = 0;
		}
		
		if(!((driver_info_t *)(drv_devices[dev]->drv_info))->init(drv_devices[dev])) {
			// remove device
			kfree(drv_devices[dev]);
			drv_devices[dev] = NULL;
		} else {
			register_interrupt_handler(drv_devices[dev]->int_no, drv_int_handler);
		}
	}
}

int32_t drv_global_mouse_x, drv_global_mouse_y;
uint8_t drv_global_mouse_btn;
bool drv_global_has_last_changed;

char drv_global_keyboard_buffer[KBUFFER * 2];
uint32_t drv_global_keyboard_index;

void drv_mouse_update(device_info *dev, int32_t xoff, int32_t yoff, uint8_t btn) {
	
	if ((xoff < 0 && drv_global_mouse_x > 0) || xoff > 0)
		drv_global_mouse_x += xoff;
	if ((yoff > 0 && drv_global_mouse_y > 0) || yoff < 0)
		drv_global_mouse_y -= yoff;
	drv_global_mouse_btn = btn;
	drv_global_has_last_changed = true;
	
	input_device_data_t *d = (input_device_data_t *)dev->type_data;
	if ((yoff > 0 && d->mouse_x > 0) || xoff > 0)
		d->mouse_x += xoff;
	if ((yoff > 0 && d->mouse_y > 0) || yoff < 0)
		d->mouse_y -= yoff;
	d->mouse_btn = btn;
	d->mouse_changed = true;
	
}

bool drv_mouse_info_global(mouse_info_t *dest) {
	dest->x = drv_global_mouse_x;
	dest->y = drv_global_mouse_y;
	dest->btn = drv_global_mouse_btn;
	if (drv_global_has_last_changed) {
		drv_global_has_last_changed = false;
		return true;
	} else {
		return false;
	}
}

bool drv_mouse_info(uint32_t id, mouse_info_t *dest) {
	uint32_t i = 0;
	while(i < MAX_DEVICES && drv_devices[i] && drv_devices[i]->id != id) i++;
	
	if (i == MAX_DEVICES)
		return false;
	
	input_device_data_t *d = (input_device_data_t *)drv_devices[i]->type_data;
	dest->x = d->mouse_x;
	dest->y = d->mouse_y;
	dest->btn = d->mouse_btn;
	if (d->mouse_changed) {
		d->mouse_changed = false;
		return true;
	} else {
		return false;
	}
}

void drv_keyboard_update(device_info *dev, char c) {
	early_mesg(LEVEL_DBG, "drv", "keyboard update, pressed '%c'", c);

	input_device_data_t *d = (input_device_data_t *)dev->type_data;
	if (d->index < KBUFFER) {
		d->buffer[d->index] = c;
		d->index++;
	}
	
	if (drv_global_keyboard_index < KBUFFER * 2) {
		drv_global_keyboard_buffer[drv_global_keyboard_index] = c;
		drv_global_keyboard_index++;
	}
	
}

char drv_kbd_char_global() {
	if (drv_global_keyboard_index > 0) {
		return drv_global_keyboard_buffer[--drv_global_keyboard_index];
	}
	return 0;
}

char drv_kbd_char(device_info_t *dev) {
	input_device_data_t *d = (input_device_data_t *)dev->type_data;
	if (d->index > 0) {
		return d->buffer[--d->index];
	}
	return 0;
}

