#ifndef SYSCALL
#define SYSCALL

#include <interrupt/isr.h>
#include <tty/tty.h>
#include <kbd/ps2kbd.h>

void syscall_init();

#endif