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
typedef struct
{
    uint8_t API_code;
    uint8_t resv[511];
} tVIREO_API;

typedef struct
{
    char signature[8]; // VIREOSYS
    uint16_t proc_ID;
    uint32_t *shared_memory;
    uint32_t *program_end;
    uint32_t *allocation_space_end;

    uint8_t resv[497];
} tVIREO_INIT_RESPONSE;

/* shared memory is 512 bytes long, it is used for kernel calls */
uint32_t *shared_memory, *shared_memory_backup;
uint32_t *allocation_space_end, *allocation_space_end_backup;
uint32_t *program_end, *program_end_backup;

void vireo_init_api()
{
    /* Inits the API, for example we need to know where our shared memory is */

    tVIREO_API API_INIT;
    tVIREO_INIT_RESPONSE *init = &API_INIT;

    API_INIT.API_code = 0; //API init
    
    __asm__ __volatile__("mov %0, %eax\n" "int $3" : : "r"(&API_INIT));

    shared_memory = shared_memory_backup = init->shared_memory;
    allocation_space_end = allocation_space_end_backup = init->allocation_space_end;   
    program_end = init->program_end;
}

static bool vireo_checks()
{
    /* checks if everything is still alright, to prevent annoying things like writing to anotherones memomry adresses 
        since Vireo doesn't have paging yet, this won't work too well, but it at least does something */

    /* returns true if GO-FOR-LAUNCH */ 

    /* I'll admit that this is a bit paranoid, but it can always happen...... */
    if(shared_memory != shared_memory_backup)
        return false;

    if(allocation_space_end != allocation_space_end_backup)
        return false;

    if(program_end != program_end_backup)
        return false;
    
    /* more to come! */

    return true;
}


void *malloc(size_t size)
{
    /* tries to give a block within the memory field of the program */
    
    if(!vireo_checks) return 0xFFFFFFFF; /* ha! try to alloc something there my friend! lol */

   /* todo */

}