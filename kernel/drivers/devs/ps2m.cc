#include "ps2m.h"
#include <kheap/heap.h>

#define PS2_DATA_PORT 0x60
#define PS2_COMM_PORT 0x64
#define PS2_STAT_PORT 0x64

#define bittest(var,pos) ((var) & (1 << (pos)))
#define ps2_wait_ready()    {while(bittest(inb(PS2_STAT_PORT), 1));}
#define ps2_wait_response() {while(bittest(inb(PS2_STAT_PORT), 0));}

bool ps2m_init(device_info_t *);
bool ps2m_deinit(device_info_t *);
bool ps2m_interrupt(device_info_t *);
bool ps2m_detect(device_info_t *);

driver_info_t ps2m_info_struct = {
	.exists = true,
	.type = INPUT_DEVICE,
	.type_funcs = NULL,
	
	.bus = BUS_SELF,
	.bus_info = NULL,
	
	.int_no = 0x2C,
	.init = ps2m_init,
	.deinit = ps2m_deinit,
	.interrupt = ps2m_interrupt,
	.detect = ps2m_detect,
	.name = "ps2m",
};

driver_info_t *ps2m_info() {
	return &ps2m_info_struct;
}

struct info {
	uint8_t which, first, second, third;
};

bool ps2m_init(device_info_t *dev) {
	early_mesg(LEVEL_DBG, "ps2m", "init");
	
	ps2_wait_ready();
	outb(0x64, 0xD4);
	ps2_wait_ready();
	outb(0x60, 0xFF);

	outb(0x64, 0x20);
	uint8_t byte = inb(0x60);

	byte |= 2;
	byte &= ~(0x20);

	outb(0x64, 0x60);
	outb(0x60, byte);

	ps2_wait_ready();
	outb(0x64, 0xD4);
	ps2_wait_ready();
	outb(0x60, 0xF4);
	
	dev->device_data = kmalloc(sizeof(struct info));
	
	return true;
}

bool ps2m_deinit(device_info_t *dev) {
	early_mesg(LEVEL_DBG, "ps2m", "deinit");
	
	kfree(dev->device_data);
	
	return true;
}

bool ps2m_interrupt(device_info_t *dev) {
	//early_mesg(LEVEL_DBG, "ps2m", "interrupt");
	struct info *a = (struct info *)dev->device_data;
	
	if (!(inb(0x64) & 0x20)) {
		return true;
	}

	uint8_t state = inb(0x60);
	
	if (a->which == 0) {
		a->first = state;
		a->which++;
		if (!bittest(state, 3)) {
			a->which = 0;
			return true;
		}
	} else if (a->which == 1) {
		a->second = state;
		a->which++;
	} else if (a->which == 2) {
		a->third = state;
		a->which = 0;
		
		int16_t mouse_x_move = 0;
		int16_t mouse_y_move = 0;

		if (bittest(a->first, 4))
			mouse_x_move = (int8_t)a->second;
		else
			mouse_x_move = a->second;
			
		if (bittest(a->first, 5))
			mouse_y_move = (int8_t)a->third;
		else
			mouse_y_move = a->third;

		drv_mouse_update(dev, mouse_x_move, mouse_y_move, a->first & 0x3);

	}

	return true;
}

bool ps2m_detect(device_info_t *dev) {
	early_mesg(LEVEL_DBG, "ps2m", "detect");
	
	// TODO
	
	return true;
}
