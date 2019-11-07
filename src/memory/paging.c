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

#include "paging.h"
#include "memory.h"

#include "../include/types.h"

#ifndef QUIET_KERNEL
#include "../screen/screen_basic.h"
#endif

void paging_init(void)
{
    uint32_t i;
    uint32_t *page_directory = (uint32_t *) 0x200000;
    uint32_t *page_table     = (uint32_t *) 0x201000;

    for(i = 0; i < 1024; i++)
        page_directory[i] = (uint32_t ) 0x02;

    page_directory[1023] = (uint32_t) (&page_directory[0]);

    for(i = 0; i < 1024; i++)
        page_table[i] = (uint32_t ) ((i * 0x1000) | 3);

    page_directory[0] = (uint32_t) &page_table[0] | 3;

    /* these are temporary so they won't get casts */
    #ifndef QUIET_KERNEL
    trace("[PAGING] &page_table -> 0x%x\n", page_table);
    trace("[PAGING] page_directory -> 0x%x\n", page_directory[0]);
    #endif
    
    ASM_CPU_PAGING_ENABLE(&page_directory[0]);

    #ifndef QUIET_KERNEL
    print((char *) "[PAGING] Hello paging world! :)\n\n");
    #endif

    trace("[PAGING] physical address of virtual address 0x8b000: 0x%x\n", paging_vptr_to_pptr(0x800000));
}

void *paging_vptr_to_pptr(void *vptr)
{
    /* location pdindex: vptr / (1024 * 1024) */
    uint32_t pdindex = (uint32_t)vptr >> 22;

    /* location ptindex: vptr / 4096 */
    uint32_t ptindex = (uint32_t) ( ((uint32_t)vptr) >> 12) & 0x03FF;

    /* TODO: change pt and pd to the real adresses when semi-finished */
    uint32_t *pd = 0x200000;
    uint32_t *pt = 0x201000;
    
    if((pd[pdindex] & 0x01) && (pt[ptindex] & 0x01))
        return (pt[ptindex] & ~0xFFF) + ((uint32_t)vptr & 0xFFF); 

    /* when we get here, the vptr is probably the pptr (because we didn't page it) */
    return vptr;
}