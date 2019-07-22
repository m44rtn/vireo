#ifndef __KERNEL_H__
#define __KERNEL_H__

/* for people who like to use the system compiler for a kernel */
#if defined(__linux__)
#error "This kernel expects to be compiled using a cross compiler"
#endif

/* (lets hope no one uses Windows or Mac...) */

#if !defined(__i386__)
#error "This is an i386 kernel, as such it expects an i386 compiler"
#endif

void cmain(void);

#endif