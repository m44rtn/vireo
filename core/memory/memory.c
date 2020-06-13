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

#include "memory.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../boot/loader.h"
#include "../boot/multiboot.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#include "../kernel/panic.h"

#include "../util/util.h"
#include "../dbg/dbg.h"

#define MEMORY_MMAP_TYPE_VIREO 1
#define MEMORY_MMAP_TYPE_RESV  2

#define MEMORY_KERNELSTRT         0x100000
#define MEMORY_MALLOC_MEMSTRT     0x200000

#define MEMORY_TABLE_LENGTH    128 

#define MEMORY_VMALLOC_STAT_ALLOCT      1<<7
#define MEMORY_VMALLOC_STAT_READONLY    1<<6

typedef struct
{
    uint8_t type;
    uint32_t loc_start;
    uint32_t loc_end;
} MEMORY_MAP;

typedef struct
{
    uint32_t available_memory; /* in kibibytes */
    uint32_t *usable_memory;   /* is an array */
    uint32_t *end_of_kernel_memory; 
    uint32_t vmemory_table_size; /* in pages (aka in array length) */
} MEMORY_INFO;

typedef struct
{
    uint32_t loc;
    uint32_t size;
} MEMORY_TABLE;


/* amazing, so if we have 12MB of memory in our system we need 6KB to store all information on all pages */
typedef struct 
{
    uint8_t stat;

    uint8_t pid;
} __attribute__ ((packed)) VIRTUAL_MEMORY_TABLE;


/* I'm sorry for this ugly define line here */
#define MEMORY_VIRTUAL_TABLES  MEMORY_MALLOC_MEMSTRT + (512U * MEMORY_TABLE_LENGTH)
/* ---- */

extern void start(void);
extern void STACK_TOP(void);

MEMORY_INFO  memory_info_t;
MEMORY_MAP   temp_memory_map[2];

/* 1 KiB of information, though I can imagine this being too little.
This can store 64 KiB of memory this way */
MEMORY_TABLE memory_t[MEMORY_TABLE_LENGTH]; 
VIRTUAL_MEMORY_TABLE *virtual_memory_t = NULL;

uint32_t virtual_memory_table_size;
uint8_t loader_type = 0;

static void memory_update_table(uint8_t index, uint32_t loc, uint8_t blocks);
static void memory_create_temp_mmap(void);

uint8_t memory_init(void)
{
    LOADER_INFO infoStruct;
    
    loader_type = loader_get_type();
    
    /* we don't know this yet */
    memory_info_t.end_of_kernel_memory = NULL;

    /* If the loader type is unknown, we won't get an info struct.
       TODO: implement int 15h ax=0xe820
       FIXME: remove not implemented exit code after implementing int 15h */
    if(loader_type == LOADER_TYPE_UNKNOWN)
        return EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;

    infoStruct = loader_get_infoStruct();
    
    /* GRUB returns KB's but I like KiB's more */
    memory_info_t.available_memory = (uint32_t) (infoStruct.total_memory * 1.024);
    
    #ifndef NO_DEBUG_INFO
    trace((char *)"[MEMORY] Total memory: %i KiB\n", memory_info_t.available_memory);
    trace((char *)"[MEMORY] Memory map location: %x\n", (unsigned int) infoStruct.mmap);
    trace((char *)"[MEMORY] Memory map length: %i bytes\n\n", infoStruct.mmap_length);
    #endif

    /* TODO: if exists, read memory map */
    /* TODO: if not exists, try int 15h (v86) */
    memory_create_temp_mmap();
    
    memset((char *) &memory_t, sizeof(MEMORY_TABLE)*128, 0);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

void *memory_paging_tables_loc(void)
{
    uint32_t *tables = (uint32_t *) MEMORY_VIRTUAL_TABLES;
    
    /* this puts the total available memory at the location of the tables allocation, which 
    save space in memory and this file (I'm too lazy to make a struct for this) */
    tables[0] = memory_info_t.available_memory;

    return (void *) &tables[0];
}

void memory_paging_final_report(uint32_t memory_used)
{
    uint32_t i = 0;
    uint32_t pages;

    /* someone took over, oh no! (or just something went horribly wrong while writing the
    code for this kernel). you can never be too cautious... right? */
    if(virtual_memory_t != NULL)
        easy_panic(PANIC_TYPE_INIT_ERROR, "2ND_PAGING_REPORT");
    
    virtual_memory_t = MEMORY_VIRTUAL_TABLES + memory_used;
    virtual_memory_table_size = ((memory_info_t.available_memory * 1000U) / 4096U) * sizeof(VIRTUAL_MEMORY_TABLE);

    /* calculate the end of the kernel memory... */
    /* TODO: maybe have vmemory_table_size be constant and a function which you can use to initialize it? */
    memory_info_t.end_of_kernel_memory = (uint32_t *) (((uint32_t) virtual_memory_t) + virtual_memory_table_size); 
    memory_info_t.vmemory_table_size = ((memory_info_t.available_memory * 1000U) / 4096U);

    trace("CR3 # bytes: %i\n", memory_used);
    trace("virtual_memory_tables: 0x%x\n", MEMORY_VIRTUAL_TABLES);
    trace("vmem table_size: %i\n", virtual_memory_table_size);
    trace("end of kernel memory: 0x%x\n", memory_info_t.end_of_kernel_memory);
    
    pages = (((uint32_t)memory_info_t.end_of_kernel_memory) % 512) ? 1 : 0;
    trace("pages: %i\n", pages);
    pages = pages + (((uint32_t)memory_info_t.end_of_kernel_memory) / 4096U);
    trace("pages: %i\n", pages);
    trace("pages memory end: 0x%x\n", pages*4096U);
    
    /* reserve kernel memory */
    /* TODO: DEFINE KERNEL_PID SOMEHWERE */
    for(i = 0; i < pages; ++i)
    {
        virtual_memory_t[i].stat = MEMORY_VMALLOC_STAT_ALLOCT | MEMORY_VMALLOC_STAT_READONLY;
        virtual_memory_t[i].pid = 0x00; /* kernel pid == 0x00 */

    }

    trace("i: %i\n", i);

    sleep(300);
}

/* FIXME: virtual not implemented 
TODO: rename 'readOnly' to something else and have it be a few bits of info about various things :) 
FYI: with readOnly I mean 'readOnly but the kernel is allowed to ignore that since it's the ruling, governing, all-knowing dictator of this machine, so why should it honor that. */
void *vmalloc(size_t size, uint8_t pid, uint8_t readOnly)
{
    /* TOD: make this actually use the features given to us by the CPU for paging */
    uint8_t blocks, available;
    uint32_t i, len;
    void *theSpace;

    /* when this happens we've got quite a problem... */
    if (virtual_memory_t == NULL)
        easy_panic(PANIC_TYPE_INIT_ERROR, "VIRTUAL_MEMORY_NOT_INITIALIZED");
    
    /* how many pages do we need? */
    blocks = (size % 4096U) ? 1U : 0U;
    blocks = (uint8_t) (blocks + (size / 4096U));

    /* find a place with enough pages free to store our data */
    for(i = 0; i < memory_info_t.vmemory_table_size; ++i)
    {
        if(!(virtual_memory_t[i].stat & MEMORY_VMALLOC_STAT_ALLOCT))
            available++;
        else
            available = 0;
        
        if(available == blocks)
            break;
    }

    /* low on memory... we can't store your request, unfortunately :/ */
    if(i >= memory_info_t.vmemory_table_size && available < blocks)
        return NULL;

    /* if we get here we DID find a place to store your stuff, so let's make sure you get it */
    len = i;    
    theSpace = (void *)((len - blocks + 1) * 4096U);
    
    /* store everything we know about the page */
    for(i = (len - blocks + 1); i < (len + blocks); ++i)
    {
        virtual_memory_t[i].stat = (MEMORY_VMALLOC_STAT_ALLOCT) | (readOnly ? MEMORY_VMALLOC_STAT_READONLY : 0);
        virtual_memory_t[i].pid  = pid;
    }

    return theSpace;
}

/* FIXME: NOT IMPLEMENTED 
also an argument should be for the size of the data at the pointer, so that it we don't demalloc the *entire* memory pool of the process */
void vfree(void){}

void *kmalloc(size_t size)
{
    uint32_t location;
    uint8_t loc, available = 0;
    uint8_t index;
    int8_t mallocd_index;

    uint8_t blocks = (size % 512) ? 1 : 0;
    blocks = (uint8_t) (blocks + (size / 512));
    

    /* see if we can find enough 512 blocks to fit our needs; look down below for a *very detailed* explanation
     on why this only allocates 512 byte blocks */
    for(loc = 0; loc < MEMORY_TABLE_LENGTH; loc++)
    {
        if(!memory_t[loc].loc)
            available++;
        else
            available = 0;

        if(available == blocks)
            break;
    }

    /* not enough free memory */
    if(loc >= MEMORY_TABLE_LENGTH && available < blocks)
        return (void *) NULL;

    /* calc the index for the memory table */
    loc = (uint8_t) (loc - blocks + 1);
    index = loc;

    mallocd_index = (int8_t) ((index - 1 < 0)? -1 : (index - 1));

    /* store the information in the memory table */
    if(mallocd_index <= -1)
        memory_update_table(0, MEMORY_MALLOC_MEMSTRT, blocks);
    else
    {
        location = (uint32_t) (memory_t[mallocd_index].loc + (memory_t[mallocd_index].size * 512));
        memory_update_table(index, location, blocks);
    }

    /* all done! :) */
    return (void *) memory_t[index].loc;

    /* alright! seems to work :)
        So, you may be asking: 'why only use 512 byte blocks?!'
        Well, I hope that the memory allocation this way is easier and more reliable than with Vireo-I */

}

void kfree(void *ptr)
{
    uint8_t i;

    size_t size;
    uint8_t len;
    
    /* find the memory table entries that correspond to 
        the pointer */
    for(i = 0; i < MEMORY_TABLE_LENGTH; i++)
        if(memory_t[i].loc == (uint32_t) ptr)
            break;
    
    size = memory_t[i].size * 512;

    len = (uint8_t) (i + memory_t[i].size);
    for(i = i; i < len; i++)
    {
        memory_t[i].loc  = 0;
        memory_t[i].size = 0;
    }
}

uint32_t memory_getAvailable(void)
{
    return memory_info_t.available_memory;
}

uint32_t memory_getKernelStart(void)
{
    return (uint32_t) MEMORY_KERNELSTRT;
}

uint32_t memory_getMallocStart(void)
{
    return (uint32_t) MEMORY_MALLOC_MEMSTRT;
}

uint32_t *memsrch(void *match, size_t matchsize, uint32_t start, uint32_t end)
{
    uint32_t i, j = 0;
    uint8_t *buffer = (uint8_t *) start;
    uint8_t *mtch = (uint8_t *) match;

    for(i = 0; i < (end - start); ++i)
    {
        if((buffer + i) == match)
            continue;

        if((mtch[j] == buffer[i]) && (j < matchsize))
            ++j;
        else if(mtch[j] != buffer[i] && j < matchsize)
            j = 0;
        else
            break;
    }

    if( (uint32_t)(buffer + i) == end)
        return NULL;

    return (uint32_t *) (buffer + i - matchsize);
}

static void memory_update_table(uint8_t index, uint32_t loc, uint8_t blocks)
{
    uint8_t i;
    uint8_t end = (uint8_t) (index + blocks);

    for(i = index; i < end; i++)
    {
        memory_t[i].loc = (uint32_t) loc;
        memory_t[i].size = blocks;
    }
}

static void memory_create_temp_mmap(void)
{

    /* where Vireo lives, sort of */
    temp_memory_map[0].type = MEMORY_MMAP_TYPE_VIREO;
    temp_memory_map[0].loc_start = (uint32_t) (((uint32_t)start) - 0x0C);
    temp_memory_map[0].loc_end   = (uint32_t) STACK_TOP;

    /* kernel flows through malloc memory */
    dbg_assert((((uint32_t) STACK_TOP) <= MEMORY_MALLOC_MEMSTRT));
    

    /* and here is the grub memory info */
    temp_memory_map[1].type = MEMORY_MMAP_TYPE_RESV;
    temp_memory_map[1].loc_start = (uint32_t) loader_get_multiboot_info_location();
    temp_memory_map[1].loc_end   = (uint32_t) sizeof(multiboot_info_t);
}
