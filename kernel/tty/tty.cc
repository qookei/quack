#include "tty.h"

void (*tty_dev)(tty_op,uint32_t);

void tty_init(void) {
	tty_dev(tty_op::INIT, 0);
}

void tty_setdev(void (*io_op)(tty_op,uint32_t)) {
	tty_dev = io_op;
}

void tty_putchar(char c) {
	tty_dev(tty_op::WRITE_CHAR, c);
}

void tty_putstr(const char *s) {
	tty_dev(tty_op::WRITE_STRING, (uint32_t)s);
}

