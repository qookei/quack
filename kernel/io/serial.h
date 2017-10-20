#ifndef SERIAL_H
#define SERIAL_H

#include "ports.h"

#define PORT 0x3F8

void serial_init(void);
uint8_t serial_read_byte(void);
void serial_write_byte(uint8_t);


#endif