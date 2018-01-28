#ifndef KSHELL
#define KSHELL

#include "../vsprintf.h"
#include "../kbd/ps2kbd.h"
#include "../multiboot.h"

void kshell_main(multiboot_info_t *d);

#endif