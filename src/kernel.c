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


#include "include/kernel_info.h"
#include "include/types.h"

#include "screen/screen_basic.h"

/* for people who like to use the system compiler for a kernel */
#if defined(__linux__)
#error "This kernel expects to be compiled using a cross compiler"
#endif

/* (lets hope no one uses Windows or Mac...) */

#if !defined(__i386__)
#error "This is an i386 kernel, as such it expects an i386 compiler"
#endif

void main(void);

void main(void)
{

    /* TODO: ASM FUNCTIONS --> in C or in assembly? */
    /* TODO: USE NASM instead of GAS */
    int i = 0;
    i++;

}