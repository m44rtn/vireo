#ifndef SYS_H
#define SYS_H

#include "../include/types.h"

uint8_t inb(uint16_t _port);
void outl(uint16_t _port, uint32_t _data);
long inl(uint16_t _port);
uint16_t inw(uint16_t port);
long sysinlong(uint16_t _port);
void outb(uint16_t _port, uint8_t _data);
void outw (uint16_t _port, uint16_t _data);

void outsw(uint16_t port, uint16_t *data, uint32_t size);
#endif
