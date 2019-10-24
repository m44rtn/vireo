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
#include "../include/global_exit_codes.h"

#include "../boot/loader.h"
#include "../boot/multiboot.h"

#include "../screen/screen_basic.h"

#include "../util/util.h"

#define MEMORY_MMAP_TYPE_VIREO 1
#define MEMORY_MMAP_TYPE_RESV  2

typedef struct
{
    uint8_t type;
    uint32_t loc_start;
    uint32_t loc_end;
} MEMORY_MAP;

typedef struct
{
    uint32_t available_memory; /* in bytes */
    uint32_t *usable_memory;   /* is an array */
} MEMORY_INFO;

typedef struct
{
    uint32_t *loc;
    uint32_t size;
} MEMORY_TABLE;

extern void start(void);
extern void STACK_TOP(void);

MEMORY_INFO  memory_info_t;
MEMORY_MAP   temp_memory_map[2];

/* 1 KiB of information, though I can imagine this being too little.
This can store 64 KiB of memory this way */
MEMORY_TABLE memory_table[128]; 

uint8_t loader_type = 0;

static void memory_create_temp_mmap(void);

uint8_t memory_init(void)
{
    LOADER_INFO infoStruct;
    
    loader_type = loader_get_type();

    /* If the loader type is unknown, we won't get an info struct.
       TODO: implement int 15h ax=0xe820
       FIXME: remove general fail exit code after implementing int 15h */
    if(loader_type == LOADER_TYPE_UNKNOWN)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    infoStruct = loader_get_infoStruct();
    
    /* GRUB returns KB's but I like KiB's more */
    memory_info_t.available_memory = (uint32_t) (infoStruct.total_memory * 1.024);

    trace((char *)"[MEMORY] Total memory: %i KiB\n", memory_info_t.available_memory);
    trace((char *)"[MEMORY] Memory map location: %x\n", (unsigned int) infoStruct.mmap);
    trace((char *)"[MEMORY] Memory map length: %i bytes\n", infoStruct.mmap_length);

    /* TODO: if exists, read memory map */
    /* TODO: if not exists, try int 15h */
    memory_create_temp_mmap();
    
    memset((char *) &memory_table, sizeof(MEMORY_TABLE)*128, 0);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

/* FIXME: virtual not implemented */
void vmalloc(void)
{}

void malloc(size_t size)
{
    uint8_t loc, available = 0;
    uint8_t index;

    uint32_t blocks = (size % 512) ? 1 : 0;
    blocks = blocks + (size / 512);

    /* see if we can find enough 512 blocks to fit our needs */
    for(loc = 0; loc < 128; loc++)
    {
        if(!memory_table[loc].loc)
            available++;
        else
            available = 0;

        if(available == blocks)
            break;
    }

    /* not enough free memory */
    if(loc >= 128)
        return NULL;

    /* calc the index for the memory table */
    loc = loc - blocks + 1;
    index = loc;

    /* TODO:
       1. calc location
       2. store information for all blocks
       3. return location */

}

static void memory_create_temp_mmap(void)
{
    /* where Vireo lives, sort of */
    temp_memory_map[0].type = MEMORY_MMAP_TYPE_VIREO;
    temp_memory_map[0].loc_start = (uint32_t) (start - 0x0C);
    temp_memory_map[0].loc_end   = (uint32_t) STACK_TOP;

    /* and here is the grub memory info */
    temp_memory_map[1].type = MEMORY_MMAP_TYPE_RESV;
    temp_memory_map[1].loc_start = (uint32_t) loader_get_multiboot_info_location();
    temp_memory_map[1].loc_end   = (uint32_t) sizeof(multiboot_info_t);
}
