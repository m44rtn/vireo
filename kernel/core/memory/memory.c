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

#include "memory.h"

#include "paging.h" 

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../include/types.h"
#include "../include/exit_code.h"
#include "../include/macro.h"

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

// 1  MiB - 1 byte reserved for kernel (0x100000 thru 0x1fffff)
#define MEMORY_KERNELSTRT         0x100000
#define MEMORY_KMALLOC_END     0x200000

#define MEMORY_TABLE_LENGTH    128U  // entries
#define MEMORY_BLOCK_SIZE      512U // bytes

#define MEMORY_MALLOC_SPACE     MEMORY_KMALLOC_END - (MEMORY_BLOCK_SIZE * (MEMORY_TABLE_LENGTH + 1))

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
    uint32_t available_memory; /* in kilobytes */
    uint32_t available_memory_bytes; /* in bytes */
    uint32_t *usable_memory;   /* is an array */
    uint32_t *end_of_kernel_memory; 
    uint32_t vmemory_table_size; /* in pages (aka in array length) */
} MEMORY_INFO;

typedef struct
{
    uint32_t loc;
    uint32_t size;
} MEMORY_TABLE;

// api stuff
typedef struct valloc_t
{
    syscall_hdr_t hdr;
    size_t size;
} __attribute__((packed)) valloc_t;

typedef struct vfree_t
{
    syscall_hdr_t hdr;
    void *ptr;
} __attribute__((packed)) vfree_t;

typedef struct api_mem_info_t
{
    size_t memory_space_kb;
    void *program_space_start;
} __attribute__((packed)) api_mem_info_t;
// -- end api stuff

/* I'm sorry for this ugly define line here */
#define MEMORY_VIRTUAL_TABLES  MEMORY_KMALLOC_END + (MEMORY_BLOCK_SIZE * MEMORY_TABLE_LENGTH)
/* ---- */

extern void start(void);
extern void STACK_TOP(void);

MEMORY_INFO  memory_info_t;
MEMORY_MAP   temp_memory_map[2];

/* 1 KiB of information, though I can imagine this being too little.
This can store 64 KiB of memory this way */
MEMORY_TABLE memory_t[MEMORY_TABLE_LENGTH]; 

uint32_t virtual_memory_table_size;
uint8_t loader_type = 0;

static void memory_update_table(uint8_t index, uint32_t loc, uint8_t blocks);
static void memory_create_temp_mmap(void);

void memory_api(void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *) req;

    switch(hdr->system_call)
    {
        case SYSCALL_GET_MEM_INFO:
        {
            api_mem_info_t *mem = (api_mem_info_t *) evalloc(sizeof(api_mem_info_t), prog_get_current_running());
            mem->memory_space_kb = memory_info_t.available_memory;
            mem->program_space_start = NULL; // TODO: either fix or remove
            
            hdr->response_ptr = mem;
            hdr->response_size = sizeof(api_mem_info_t);
            break;
        }

        case SYSCALL_VALLOC:
        {
            valloc_t *v = (valloc_t *) req;
            v->hdr.response_ptr = evalloc(v->size, prog_get_current_running());
            break;
        }

        case SYSCALL_VFREE:
        {
            vfree_t *v = (vfree_t *) req;
            
            ASSERT(v);

            if((uint32_t)v->ptr < MEMORY_KMALLOC_END)
                { v->hdr.exit_code = EXIT_CODE_GLOBAL_OUT_OF_RANGE; break;}
            if((uint32_t)v->ptr > memory_getAvailable())
                { v->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break;}
            
            vfree(v->ptr);
            break;
        }

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}

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
    
    /* GRUB returns KB's but I like KiB's more, sorry */
    memory_info_t.available_memory = (uint32_t) (infoStruct.total_memory);
    memory_info_t.available_memory_bytes = memory_info_t.available_memory * 1000;
    
    #ifndef NO_DEBUG_INFO
    print_value("[MEMORY] Total memory: %i KiB\n", memory_info_t.available_memory);
    print_value("[MEMORY] Memory map location: %x\n", (unsigned int) infoStruct.mmap);
    print_value("[MEMORY] Memory map length: %i bytes\n\n", infoStruct.mmap_length);
    #endif

    /* TODO: if exists, read memory map */
    /* TODO: if not exists, try int 15h (v86) */
    memory_create_temp_mmap();
    
    memset((void *) &memory_t[0], sizeof(MEMORY_TABLE)*MEMORY_TABLE_LENGTH, 0);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

uint32_t *memory_paging_tables_loc(void)
{
    uint32_t *tables = (uint32_t *) (MEMORY_VIRTUAL_TABLES);

    /* this puts the total available memory at the location of the tables allocation, which 
    save space in memory and this file (I'm too lazy to make a struct for this) */
    tables[0] = memory_info_t.available_memory;

    return tables;
}

void *kmalloc(size_t size)
{
    uint32_t location;
    uint8_t loc, available = 0;
    uint8_t index;

    uint8_t blocks = (uint8_t) (HOW_MANY(size, MEMORY_BLOCK_SIZE));

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
    loc = (uint8_t) (loc - (blocks - 1));
    index = loc;

    /* store the information in the memory table */
    location = (uint32_t) (MEMORY_MALLOC_SPACE + (loc * MEMORY_BLOCK_SIZE));
    memory_update_table(index, location, blocks);

    /* all done! :) */
    return (void *) memory_t[index].loc;

    /* alright! seems to work :)
        So, you may be asking: 'why only use 512 byte blocks?!'
        Well, I hope that the memory allocation this way is easier and more reliable than with Vireo-I */

}

void kfree(void *ptr)
{
    uint8_t i;

    ASSERT(ptr);
    ASSERT((uint32_t)ptr < MEMORY_KMALLOC_END);

    if(ptr == NULL)
        return;
    
    /* find the memory table entries that correspond to 
        the pointer */
    for(i = 0; i < MEMORY_TABLE_LENGTH; i++)
        if(memory_t[i].loc == (uint32_t) ptr)
            break;

    ASSERT(i < MEMORY_TABLE_LENGTH);

    size_t s = memory_t[i].size;

    memset(ptr, s * MEMORY_BLOCK_SIZE, 0);
    memset(&memory_t[i], sizeof(MEMORY_TABLE) * s, 0);
}

uint32_t memory_getAvailable(void)
{
    return memory_info_t.available_memory_bytes;
}

uint32_t memory_getKernelStart(void)
{
    return (uint32_t) MEMORY_KERNELSTRT;
}

uint32_t memory_getMallocStart(void)
{
    return (uint32_t) MEMORY_MALLOC_SPACE;
}

uint32_t memory_get_malloc_end(void)
{
    return (uint32_t) MEMORY_KMALLOC_END;
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
    ASSERT((((uint32_t) STACK_TOP) <= MEMORY_KMALLOC_END));
    

    /* and here is the grub memory info */
    temp_memory_map[1].type = MEMORY_MMAP_TYPE_RESV;
    temp_memory_map[1].loc_start = (uint32_t) loader_get_multiboot_info_location();
    temp_memory_map[1].loc_end   = (uint32_t) sizeof(multiboot_info_t);
}
