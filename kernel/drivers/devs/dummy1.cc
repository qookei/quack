#include "dummy1.h"

pci_info_t dummy1_pci_info_struct = {
	.vendor = 0x8086,
	.device = 0x1237,
};

bool dummy1_init(device_info_t *);
bool dummy1_deinit(device_info_t *);
bool dummy1_interrupt(device_info_t *);

driver_info_t dummy1_info_struct = {
	.exists = true,
	.type = INPUT_DEVICE,
	.type_funcs = NULL,
	
	.bus = BUS_PCI,
	.bus_info = &dummy1_pci_info_struct,
	
	.int_no = 0,
	.init = dummy1_init,
	.deinit = dummy1_deinit,
	.interrupt = dummy1_interrupt,
	.detect = NULL,
	.name = "dummy1",
};

driver_info_t *dummy1_info() {
	return &dummy1_info_struct;
}

bool dummy1_init(device_info_t *) {
	early_mesg(LEVEL_DBG, "dummy1", "driver init called!");
	return true;
}

bool dummy1_deinit(device_info_t *) {
	early_mesg(LEVEL_DBG, "dummy1", "driver deinit called!");
	return true;
}

bool dummy1_interrupt(device_info_t *) {
	early_mesg(LEVEL_DBG, "dummy1", "driver interrupt called!");
	return true;
}
