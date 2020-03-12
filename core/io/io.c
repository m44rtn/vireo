/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "io.h"

#include "../include/types.h"

uint8_t inb(uint16_t _port)
{
	uint8_t rv;

	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port) );
		
	return rv;
}

uint16_t inw(uint16_t _port)
{
	uint16_t rv;
	__asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

void outb (uint16_t _port, uint8_t _data)
{
	
	__asm__ __volatile__ ("outb %1, %0" :: "dN" (_port), "a" (_data) );
}

void outw (uint16_t _port, uint16_t _data)
{
	
	__asm__ __volatile__ ("outw %1, %0" :: "dN" (_port), "a" (_data) );
}

void outl(uint16_t _port, uint32_t _data)
{
	__asm__ __volatile__ ("outl %1, %0" :: "dN" (_port), "a" (_data) );
}

long inl(uint16_t _port)
{
	long rv;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

void outsw(uint16_t port, uint16_t *data, uint32_t size)
{
	uint32_t i;

	for(i = 0; i < size; ++i)
	{
		__asm__ __volatile__ ("outw %1, %0" :: "dN" (port), "a" (data[i]));
	}
	
}

void insw(uint16_t port, uint16_t *buffer, uint32_t size)
{
	uint32_t i;

	for(i = 0; i < size; ++i)
	{
		__asm__ __volatile__ ("inw %1, %0" :: "a" (buffer[i]), "dN" (port));
	}
}