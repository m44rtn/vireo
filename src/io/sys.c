#include "sys.h"

uint8_t inb(uint16_t _port){
	
	uint8_t rv;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

uint16_t inw(uint16_t _port){
	uint16_t rv;
	__asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

void outb (uint16_t _port, uint8_t _data){
	
	__asm__ __volatile__ ("outb %1, %0" :: "dN" (_port), "a" (_data) );
}

void outw (uint16_t _port, uint16_t _data){
	
	__asm__ __volatile__ ("outw %1, %0" :: "dN" (_port), "a" (_data) );
}

void outl(uint16_t _port, uint32_t _data){
	__asm__ __volatile__ ("outl %1, %0" :: "dN" (_port), "a" (_data) );
}

long inl(uint16_t _port){ //... except it is not a long
	long rv;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

long sysinlong(uint16_t _port){ //... except it is not a long
	long rv;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

void outsw(uint16_t port, uint16_t *data, uint32_t size)
{
	for(int i = 0; i < size; i++)
	{
		__asm__ __volatile__ ("outw %1, %0" :: "dN" (port), "a" (data[i]));
	}
	
}

