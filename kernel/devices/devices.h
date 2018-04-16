#ifndef DEVICES
#define DEVICES

#include <fs/devfs.h>
#include <io/rtc.h>

void dev_tty_init();
void dev_initrd_init();
void dev_videomode_init();
void dev_mouse_init();
void dev_uptime_init();

#endif