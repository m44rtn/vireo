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

#include "../screen/screen_basic.h"

void paging_init(void)
{
    /* so let's try some paging */
    uint32_t i;
    uint32_t *page_directory = 0x200000;
    uint32_t *page_table = 0x201000;

    for(i = 0; i < 1024; i++)
        page_directory[i] = 0x02;
    for(i = 0; i < 1024; i++)
        page_table[i] = (i * 0x1000) | 3;

    page_directory[0] = (uint32_t) &page_table[0] | 3;

    #ifndef QUIET_KERNEL
    trace("[PAGING] &page_table -> 0x%x\n", page_table);
    trace("[PAGING] page_directory -> 0x%x\n", page_directory[0]);
    #endif
    

    ASM_CPU_PAGING_ENABLE(&page_directory[0]);

    #ifndef QUIET_KERNEL
    print("[PAGING] Hello paging world! :)\n\n");
    #endif
}