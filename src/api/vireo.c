/* MIT License:

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
SOFTWARE. */

#include "vireo.h"

#include "../include/keych.h"
#include "../include/types.h"

/* this may be the worst, most macgyvered, duct-taped together API in the history of APIs */

void *malloc(size_t size)
{
    /* tries to give a block within the memory field of the program */
    uint32_t program_end;
    uint32_t stack[16];


    /* this will ask the kernel what the end of the program is and throw it on the stack 
        eax = function we want to use
        edi = location of the stack */
    __asm__ __volatile__("mov $0, %eax\n"
         "mov %0, %edi\n"
         "int $3" : :  "r" (&stack));    
    
}