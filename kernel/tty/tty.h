#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>

enum tty_op {
	INIT = 0,
	WRITE_CHAR,
	WRITE_STRING,
};

void tty_init(void);
void tty_setdev(void (*io_op)(tty_op,uint32_t));
void tty_putchar(char);
void tty_putstr(const char*);

#endif