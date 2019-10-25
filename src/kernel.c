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

#include "kernel.h"

#include "include/kernel_info.h"
#include "include/global_exit_codes.h"
#include "include/global_flags.h"
#include "include/types.h"

#include "boot/loader.h"

#include "io/io.h"

#include "screen/screen_basic.h"

#include "util/util.h"

#include "cpu/gdt.h"
#include "cpu/interrupts/IDT.h"
#include "cpu/pic.h"
#include "cpu/cpu.h"

#include "memory/memory.h"

void init_env(void);
void main(void);

/* initializes 'the environment' */
void init_env(void)
{
     /* GDT structures */
    GDT_ACCESS access;
    GDT_FLAGS flags;
    uint8_t exit_code;

    loader_detect();

    /* setup GDT structures */
    access.dataisWritable   = true;
    access.codeisReadable   = true;

    flags.Align4k           = true;
    flags.use16             = false;
    
    GDT_setup(access, flags);
    PIC_controller_setup();

    IDT_setup();
    CPU_init();
    
    exit_code = memory_init();
}

void main(void)
{
    unsigned int exit_code = 0;
    SystemInfo.GLOBAL_FLAGS = 0;  

    exit_code = screen_basic_init();
   
    if(exit_code != EXIT_CODE_GLOBAL_SUCCESS) 
        while(1);
    
    /* announce ourselves */
    trace((char *) "[VERSION] Vireo II build %i\n\n", BUILD);
    
    init_env();

    uint32_t *loc = (uint32_t *) malloc(512);
    trace("loc 512: 0x%x\n", loc);
    
    loc = (uint32_t *) malloc(789);
    trace("loc 789: 0x%x\n", loc);

    loc = (uint32_t *) malloc(119);
    trace("loc 119: 0x%x\n", loc);

    demalloc(loc);

    loc = malloc(119);
    trace("loc 119: 0x%x\n", loc);
            
    while(1);
}
