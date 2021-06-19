/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

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

#ifndef __PAGING_H__
#define __PAGING_H__

#include "../include/types.h"

#define PAGE_REQ_ATTR_READ_WRITE    1U << 0
#define PAGE_REQ_ATTR_READ_ONLY     !PAGE_REQ_ATTR_READ_WRITE
#define PAGE_REQ_ATTR_SUPERVISOR    1U << 1

typedef struct
{
    uint8_t pid;        /* process id */
    uint8_t attr;       /* paging attributes --> use defines */
    size_t size;        /* size in bytes to be allocated */
} __attribute__((packed)) PAGE_REQ;


void paging_init(void);
void *paging_vptr_to_pptr(void *vptr);
void paging_map(void *pptr, void *vptr, PAGE_REQ *req);

void *valloc(PAGE_REQ *req);
void vfree(void *ptr);

extern void ASM_CPU_PAGING_ENABLE(unsigned int *table);
extern void ASM_CPU_INVLPG(void *paddr);

#endif