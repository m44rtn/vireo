/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

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

#include "include/exit_code.h"
#include "include/types.h"

#include "boot/loader.h"

#include "io/io.h"

#include "screen/screen_basic.h"

#include "util/util.h"

#include "cpu/gdt.h"
#include "cpu/interrupts/IDT.h"
#include "cpu/cpu.h"

#include "memory/memory.h"
#include "memory/paging.h"

#include "hardware/pci.h"

#include "dbg/dbg.h"

#include "hardware/pic.h"
#include "hardware/driver.h"

#include "kernel/exec.h"
#include "kernel/panic.h"
#include "kernel/info.h"

/* TODO: remove */
#include "storage/IDE_commands.h"

void init_env(void);
void main(void);

typedef struct 
{
    uint32_t sign1;
    char *sign2;
} __attribute__((packed)) tester;


/* initializes 'the environment' */
void init_env(void)
{
     /* GDT structures */
    GDT_ACCESS access;
    GDT_FLAGS flags;
    uint8_t exit_code;

    exit_code = loader_detect();
    if(exit_code == EXIT_CODE_GLOBAL_NOT_IMPLEMENTED) 
        debug_print_warning((char *) "Support for current bootloader not implemented");

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

    paging_init();

    pci_init();

    driver_init();

}

void main(void)
{
    unsigned int exit_code = 0;
    unsigned int *devicelist, device;
    uint32_t buf;
    uint16_t *buffy;

    uint32_t *bleep;
    
    
    exit_code = screen_basic_init();
   
    if(exit_code != EXIT_CODE_GLOBAL_SUCCESS) 
        while(1);
    
    init_env();

    
    devicelist = pciGetDevices(0x01, 0x01);
    device = devicelist[1];
    demalloc(devicelist);

    bleep = malloc(512);
    bleep[0] = IDE_COMMAND_INIT;
    bleep[1] = (uint32_t) device;

    driver_exec(pciGetInfo(device) | DRIVER_TYPE_PCI, bleep);

    buffy = malloc(512);
    memset((char*)bleep, sizeof(4 * sizeof(uint32_t)), (char) 0);
    
    bleep[0] = IDE_COMMAND_READ;
    bleep[1] = 1;
    bleep[2] = 63;
    bleep[3] = 1;
    bleep[4] = (uint32_t) buffy;

    devicelist = pciGetDevices(0x01, 0x01);
    device = devicelist[1];
    demalloc(devicelist);

    trace( (char*)"bleep.command = %x\n", bleep[0]);
    trace( (char*)"device: %x\n", device);
     trace( (char*)"buffy: 0x%x\n", (uint32_t) buffy);
    
    driver_exec(pciGetInfo(device) | DRIVER_TYPE_PCI, bleep);

    trace( (char*)"boot signature: %x\n", (buffy[255]) );


    #ifndef QUIET_KERNEL /* you can define QUIET_KERNEL in types.h and it'll make all modules quiet */
    print((char*) "[KERNEL] ");
    info_print_version();
    print((char*)"\n");
    #endif
            
    while(1);
}
