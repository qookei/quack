#include "ps2k.h"
#include <kheap/heap.h>
#include <fs/vfs.h>

#define PS2_DATA_PORT 0x60
#define PS2_COMM_PORT 0x64
#define PS2_STAT_PORT 0x64

#define SC_MAX 0x57
#define SC_CAPSLOCK 0x3A
#define SC_ENTER 0x1C
#define SC_BACKSPACE 0x0E
#define SC_RIGHT_SHIFT 0x36
#define SC_LEFT_SHIFT 0x2A
#define SC_RIGHT_SHIFT_REL 0xB6
#define SC_LEFT_SHIFT_REL 0xAA
#define SC_F12 0x58
#define SC_NUMLOCK 0x45


#define bittest(var,pos) ((var) & (1 << (pos)))
#define ps2_wait_ready()    {while(bittest(inb(PS2_STAT_PORT), 1));}
#define ps2_wait_response() {while(bittest(inb(PS2_STAT_PORT), 0));}

bool ps2k_init(device_info_t *);
bool ps2k_deinit(device_info_t *);
bool ps2k_interrupt(device_info_t *);
bool ps2k_detect(device_info_t *);

driver_info_t ps2k_info_struct = {
	.exists = true,
	.type = INPUT_DEVICE,
	.type_funcs = NULL,
	
	.bus = BUS_SELF,
	.bus_info = NULL,
	
	.int_no = 0x21,
	.init = ps2k_init,
	.deinit = ps2k_deinit,
	.interrupt = ps2k_interrupt,
	.detect = ps2k_detect,
	.name = "ps2k",
};

driver_info_t *ps2k_info() {
	return &ps2k_info_struct;
}

struct ps2k_info_st {
	uint8_t num, caps, shift;
	char *ps2_low_def, *ps2_upp_sft, *ps2_upp_cap, *ps2_low_csf;
    char *ps2_low_def_num, *ps2_upp_sft_num, *ps2_upp_cap_num, *ps2_low_csf_num;
};

bool ps2k_init(device_info_t *dev) {
	early_mesg(LEVEL_DBG, "ps2k", "init");
	
	while (inb(0x64) & 0x01)
		(void)inb(0x60);
	
	dev->device_data = kmalloc(sizeof(struct ps2k_info_st));
	
	struct stat st;
	int i = stat("/kbmaps/kbmap_enUS.kbd", &st);
	if (i < 0) return false;

	char *_mapdata = (char *)kmalloc(st.st_size);
	ps2k_info_st *data = (ps2k_info_st *)dev->device_data;

	int file = open("/kbmaps/kbmap_enUS.kbd", O_RDONLY);
	
	if (file < 0) {
		kfree(_mapdata);
		printf("ps2: open failed!\n");
		return false;
	}

	int b = read(file, _mapdata, st.st_size);
	close(file);

	if (b < 0) {
		printf("ps2: read failed!\n");
		kfree(_mapdata);
		return false;
	}

	size_t load = b / 8;

	data->ps2_low_def = _mapdata;
	data->ps2_upp_sft = _mapdata + load;
	data->ps2_upp_cap = _mapdata + load * 2;
	data->ps2_low_csf = _mapdata + load * 3;

	data->ps2_low_def_num = _mapdata + load * 4;
	data->ps2_upp_sft_num = _mapdata + load * 5;
	data->ps2_upp_cap_num = _mapdata + load * 6;
	data->ps2_low_csf_num = _mapdata + load * 7;
	
	return true;
}

bool ps2k_deinit(device_info_t *dev) {
	early_mesg(LEVEL_DBG, "ps2k", "deinit");
	
	kfree(dev->device_data);
	
	return true;
}

void ps2k_set_led(bool _caps, bool _num, bool _scroll) {
	uint8_t val = 0;
	if (_scroll) val |= 1;
	if (_num)	val |= 2;
	if (_caps)	val |= 4;

	ps2_wait_ready();
	outb(PS2_DATA_PORT, 0xED);

	ps2_wait_ready();
	outb(PS2_DATA_PORT, val);
}

bool ps2k_interrupt(device_info_t *dev) {
	uint8_t key = inb(0x60);
	ps2k_info_st *data = (ps2k_info_st *)dev->device_data;
	
	if (key == SC_RIGHT_SHIFT_REL || key == SC_LEFT_SHIFT_REL)
		data->shift = false;
	else if (key == SC_RIGHT_SHIFT || key == SC_LEFT_SHIFT)
		data->shift = true;
	else if (key == SC_CAPSLOCK) {
		data->caps = !data->caps;
		ps2k_set_led(data->caps, data->num, true);
	} else if (key == SC_NUMLOCK) {
		data->num = !data->num;
		ps2k_set_led(data->caps, data->num, true);
	} else if (key < SC_MAX) {
		char c = 0;

		if (data->caps && !data->shift && !data->num)
			c = data->ps2_upp_cap[key];
		else if (!data->caps && data->shift && !data->num)
			c = data->ps2_upp_sft[key];
		else if (data->caps && data->shift && !data->num)
			c = data->ps2_low_csf[key];
		else if (!data->caps && !data->shift && !data->num)
			c = data->ps2_low_def[key];
		else if (data->caps && !data->shift && data->num)
			c = data->ps2_upp_cap_num[key];
		else if (!data->caps && data->shift && data->num)
			c = data->ps2_upp_sft_num[key];
		else if (data->caps && data->shift && data->num)
			c = data->ps2_low_csf_num[key];
		else
			c = data->ps2_low_def_num[key];

		drv_keyboard_update(dev, c);

	}
	
	return true;
}

bool ps2k_detect(device_info_t *dev) {
	early_mesg(LEVEL_DBG, "ps2k", "detect");
	
	// TODO
	
	return true;
}
