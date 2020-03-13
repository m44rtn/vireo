/*
MIT license
Copyright (c) 2019 Maarten Vermeulen

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

#ifndef __SYS_H__
#define __SYS_H__

extern void ASM_OUTB(unsigned int port, unsigned int data);
extern void ASM_OUTL(unsigned int port, unsigned int data);
extern unsigned int ASM_INB(unsigned int port);
extern unsigned int ASM_INL(unsigned short port);

extern void ASM_INSW(unsigned int port, unsigned int numWords, unsigned int buffer);
extern void ASM_IOWAIT(void);

unsigned char inb(unsigned short _port);
unsigned short inw(unsigned short _port);
void outb (unsigned short _port, unsigned char _data);
void outw (unsigned short _port, unsigned short _data);
void outl (unsigned short _port, unsigned int _data);
long inl (unsigned short _port);
void outsw(unsigned short port, unsigned int size, unsigned short *data);
void insw(unsigned short port, unsigned int size, unsigned short *buffer);

#endif