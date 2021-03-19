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

#include "paging.h"
#include "memory.h"

#include "../include/types.h"
#include "../include/macro.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#include "../dbg/dbg.h"

#include "../util/util.h"

#define PAGING_ADDR_MSK         0xFFFFF000       
#define PAGING_PAGE_SIZE        4096 /* bytes */
#define PAGING_TABLE_SIZE       1024 /* entries */

#define PAGING_TABLE_TYPE_DIR 0
#define PAGING_TABLE_TYPE_TAB 1

#define PAGE_PRESENT 1

uint32_t g_max_pages = 0;

typedef struct
{
    uint8_t pid;
} __attribute__ ((packed)) shadow_allocated;

shadow_allocated *shadow_t;
uint32_t shadow_len = 0;
uint32_t *page_dir = NULL;


static uint32_t paging_convert_ptr_to_entry(uint32_t ptr, PAGE_REQ *req);
static uint32_t *paging_find_free(uint32_t npages);
static uint32_t paging_create_tables(void);
static void paging_prepare_table(uint32_t *table, uint8_t type);
static void paging_map_kernelspace(uint32_t end_of_kernel_space);

void paging_init(void)
{

    uint32_t kernel_space_end = paging_create_tables();
    paging_map_kernelspace(kernel_space_end);
    
    ASM_CPU_PAGING_ENABLE(page_dir);

    #ifndef NO_DEBUG_INFO
    print( "[PAGING] Hello paging world! :)\n\n");
    #endif
}

void *paging_vptr_to_pptr(void *vptr)
{
    uint32_t pdindex = (uint32_t)vptr >> 22; /* same as vptr / PAGING_PAGE_SIZE / PAGING_TABLE_SIZE */

    /* location ptindex: vptr / PAGING_PAGE_SIZE */
    uint32_t ptindex = (uint32_t) (((uint32_t)vptr) >> 12) & 0x03FF;

    uint32_t *pd = page_dir;
    uint32_t *pt = (uint32_t *) (pd[pdindex] & 0xFFFFF000);
    
    if((pd[pdindex] & 0x01) && (pt[ptindex] & 0x01))
        return (void *) ((pt[ptindex] & ((uint32_t)~0xFFF)) + ((uint32_t)vptr & 0xFFF)); 

    /* when we get here, the vptr is probably the pptr (because we didn't page it, 
    else you'll probably hopefully get a PAGE_FAULT?) */
    return vptr;
}

void paging_map(void *pptr, void *vptr, PAGE_REQ *req)
{
    uint32_t pdindex = (uint32_t)vptr >> 22;
    uint32_t ptindex = (uint32_t) (((uint32_t)vptr) >> 12) & 0x03FF;

    uint32_t *pd = page_dir;
    uint32_t *pt = (uint32_t *) (pd[pdindex] & 0xFFFFF000);

    pt[ptindex] = paging_convert_ptr_to_entry((uint32_t) pptr, req);

    ASM_CPU_INVLPG((uint32_t *)pptr);
    
}

void *valloc(PAGE_REQ *req)
{   
    uint32_t npages;
    uint32_t *free_pages, *ptable;
    uint32_t page_id, t_index, d_index;
    uint32_t i; /* for-loop */

    /* just checking... */
    dbg_assert((uint32_t)page_dir);

    /* how many pages do we need? */
    npages = HOW_MANY((req->size), PAGING_PAGE_SIZE);

    if((npages > g_max_pages) || (npages == 0))
        return NULL;

    free_pages = paging_find_free(npages);
    dbg_assert(free_pages);

    for(i = 0; i < npages; ++i)
    {
        page_id = free_pages[i];

        /* calculate page table and directory index */
        d_index = page_id >> 10; /* same as page_id / PAGING_TABLE_SIZE */
        t_index = page_id % PAGING_TABLE_SIZE;

        ptable = (uint32_t *) (page_dir[d_index] & PAGING_ADDR_MSK);

        ptable[t_index] = paging_convert_ptr_to_entry(ptable[t_index], req);
        
        ASM_CPU_INVLPG((uint32_t *)ptable[t_index]);

        /* update our information about this page */
        shadow_t[page_id].pid = req->pid;

        /* store the address of that page */
        free_pages[i] = page_id * PAGING_PAGE_SIZE; 
    }

    return free_pages;
}

void vfree(void *ptr)
{
    uint32_t d_index, t_index,
            page_id = ((uint32_t) ptr) / PAGING_PAGE_SIZE;
    uint32_t *ptable;
    
    /* remove the contents
       --> note to future self when debugging: memset() has caused problems with kfree()
        before */
    memset((char *)ptr, PAGING_PAGE_SIZE, 0x00);

    d_index = page_id / PAGING_TABLE_SIZE;
    t_index = page_id % PAGING_TABLE_SIZE;

    /* make page supervisor only */
    ptable = (uint32_t *) (page_dir[d_index] & PAGING_ADDR_MSK);
    ptable[t_index] = ptable[t_index] & ~(PAGE_REQ_ATTR_SUPERVISOR<<1);

    /* update our shadow map (RESV PID means unallocated) */
    shadow_t[page_id].pid = PID_RESV;
}

/* function: find_free_pages()
   returns: - allocated pointer to an array with free pages;
            - OR NULL if failed*/ 
static uint32_t *paging_find_free(uint32_t npages)
{
    uint32_t *parray = (uint32_t *) kmalloc(npages * sizeof(uint32_t *));
    uint32_t i, index = 0;

    /* huge problem */
    if(!shadow_len)
        return NULL;

    for(i = 0; i < shadow_len; ++i)
    {
        if(shadow_t[i].pid != PID_RESV)
            continue;
        
        /* found free page */
        parray[index] = i;

        if(index == npages)
            return parray;

        ++index;
    }

    /* something went wrong */
    kfree((void *) parray);

    return NULL;
}

static uint32_t paging_convert_ptr_to_entry(uint32_t ptr, PAGE_REQ *req)
{
    /* remove the attributes so that we only enable the things we should */
    uint32_t temp = ptr & ~((PAGE_REQ_ATTR_SUPERVISOR | PAGE_REQ_ATTR_READ_ONLY) << 1);   

    /* now enable the things we do need. */ 
    temp = (uint32_t) (temp | (uint32_t)((req->attr) << 1U) | PAGE_PRESENT);

    return temp;
}

/* returns: end of the tables */
static uint32_t paging_create_tables(void)
{
    /* I like spagetthi :) */

    uint32_t available_mem, page_tables;
    uint32_t i, table_loc, amount_mem; /* for-loop */

    page_dir = memory_paging_tables_loc();

    available_mem = (page_dir[0] * 1000);

    /* here's some math; first up: the amount of page tables required to map all of the memory available */
    page_tables = (available_mem / PAGING_PAGE_SIZE); /* # of pages */

    g_max_pages = page_tables;

    page_tables = HOW_MANY(page_tables, PAGING_TABLE_SIZE); /* # page tables */

    /* next: how much memory is needed for them. */
    amount_mem = (PAGING_TABLE_SIZE + (page_tables * PAGING_TABLE_SIZE)) * sizeof(uint32_t); /* in bytes */
    
    paging_prepare_table(page_dir, PAGING_TABLE_TYPE_DIR);
    
    /* put the tables where we need them and fill them with adresses/pages */
    for(i = 1; i <= page_tables; ++i)
    {
        table_loc = (uint32_t) ((i * 0x1000) + ((uint32_t) page_dir));
        page_dir[i - 1] = (uint32_t) table_loc | 0x03;

        paging_prepare_table((uint32_t *) table_loc, PAGING_TABLE_TYPE_TAB);      
    }

    /* allocate a shadow map for all of the pages */
    shadow_len = available_mem / PAGING_PAGE_SIZE;
    shadow_t = (shadow_allocated *) kmalloc(shadow_len * sizeof(shadow_allocated));
    memset((char *)shadow_t, shadow_len, (char) PID_RESV); /* no page is allocated */

    return (((uint32_t)page_dir) + amount_mem);
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

static void paging_map_kernelspace(uint32_t end_of_kernel_space)
{
    PAGE_REQ req = {PID_KERNEL, PAGE_REQ_ATTR_SUPERVISOR, PAGING_PAGE_SIZE};
    uint32_t i;
    uint32_t pages = end_of_kernel_space >> 12; /* same as end_of_kernel_space / PAGING_PAGE_SIZE */

    for(i = 0; i < pages; ++i)
    {
        paging_map((void *) (i << 12), (void *) (i << 12), &req);
        shadow_t[i].pid = PID_KERNEL;
    }
}
