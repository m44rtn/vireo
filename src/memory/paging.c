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

#define PAGING_TABLE_TYPE_DIR 0
#define PAGING_TABLE_TYPE_TAB 1

uint32_t *page_dir = NULL;

static void paging_create_tables(void);
static void paging_prepare_table(uint32_t *table, uint8_t type);

void paging_init(void)
{
    /* current should be enough to get v86 to work 
    TODO: list of todo things:
        - paging_umap()
        - store pages on disk */  

    paging_create_tables();
    
    ASM_CPU_PAGING_ENABLE(page_dir);

    #ifndef QUIET_KERNEL
    print((char *) "[PAGING] Hello paging world! :)\n\n");
    #endif

}

void *paging_vptr_to_pptr(void *vptr)
{
    /* location pdindex: vptr / (1024 * 1024) */
    uint32_t pdindex = (uint32_t)vptr >> 22;

    /* location ptindex: vptr / 4096 */
    uint32_t ptindex = (uint32_t) (((uint32_t)vptr) >> 12) & 0x03FF;

    uint32_t *pd = page_dir;
    uint32_t *pt = (uint32_t *) (pd[pdindex] & 0xFFFFF000);
    
    if((pd[pdindex] & 0x01) && (pt[ptindex] & 0x01))
        return (void *) ((pt[ptindex] & ((uint32_t)~0xFFF)) + ((uint32_t)vptr & 0xFFF)); 

    /* when we get here, the vptr is probably the pptr (because we didn't page it, 
    else you'll probably hopefully get a PAGE_FAULT?) */
    return vptr;
}

void paging_map(void *pptr, void *vptr)
{
    uint32_t pdindex = (uint32_t)vptr >> 22;
    uint32_t ptindex = (uint32_t) (((uint32_t)vptr) >> 12) & 0x03FF;

    uint32_t *pd = page_dir;
    uint32_t *pt = (uint32_t *) (pd[pdindex] & 0xFFFFF000);

    ASM_CPU_INVLPG((uint32_t *)pptr);
    pt[ptindex] = (uint32_t) (((uint32_t)pptr) << 12)  | 0x03;
}

static void paging_create_tables(void)
{
    uint32_t available_mem, page_tables, amount_mem;
    uint32_t i, table_loc; /* for-loop */

    page_dir = memory_paging_tables_loc();
    available_mem = (page_dir[0] * 1000);

    /* here's some math; first up: the amount of page tables required to map all of the memory available */
    page_tables = (available_mem / 4096);
    page_tables = (page_tables % 1024) ? (page_tables / 1024) + 1 : page_tables / 1024;

    /* next: how much memory is needed for them */
    amount_mem = (1024 + (page_tables * 1024)) * sizeof(uint32_t);
    
    paging_prepare_table(page_dir, PAGING_TABLE_TYPE_DIR);
    
    /* put the tables where we need them */
    for(i = 1; i <= page_tables; ++i)
    {
        table_loc = (uint32_t) ((i * 0x1000) + ((uint32_t) page_dir));
        page_dir[i - 1] = (uint32_t) table_loc | 0x03;

        paging_prepare_table((uint32_t *) table_loc, PAGING_TABLE_TYPE_TAB);      
    }

    /* TODO: remember where the tables end (amount_mem), and maybe even report it to the memory module */
}


static void paging_prepare_table(uint32_t *table, uint8_t type)
{
    uint32_t i;
    static uint32_t previous_end = 0;
    
    if (type == PAGING_TABLE_TYPE_DIR)
        for(i = 0; i < 1024; ++i)
            table[i] = (uint32_t) 0x02;
    else
    {
        for(i = 0; i < 1024; ++i)
            table[i] = (uint32_t ) ((previous_end + i * 0x1000) | 3);

        previous_end = table[1023];
    }

}
