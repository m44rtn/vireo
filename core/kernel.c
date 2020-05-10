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
#include "io/diskio.h"

#include "screen/screen_basic.h"

#include "util/util.h"

#include "cpu/gdt.h"
#include "cpu/interrupts/IDT.h"
#include "cpu/cpu.h"

#include "memory/memory.h"
#include "memory/paging.h"

#include "hardware/pci.h"
#include "hardware/pic.h"
#include "hardware/driver.h"

#include "dbg/dbg.h"

#include "kernel/mbr.h"
#include "kernel/exec.h"
#include "kernel/panic.h"
#include "kernel/info.h"

/* TODO: remove */
#include "drv/COMMANDS.H"
#include "drv/IDE_commands.h"

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
    uint32_t *drvcmd, *devicelist, device; /* for the driver inits and such */

    exit_code = loader_detect();
    if(exit_code == EXIT_CODE_GLOBAL_NOT_IMPLEMENTED)
        debug_print_warning((char *) "WARNING: Support for current bootloader not implemented");

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

    /* the kernel should actually detect anything that has a driver and init them,
    but until that's implemented this'll live here */
    devicelist = pciGetDevices(0x01, 0x01);
    device = devicelist[1];
    free(devicelist);

    drvcmd = malloc(DRIVER_COMMAND_PACKET_LEN * sizeof(uint32_t *));
    drvcmd[0] = DRV_COMMAND_INIT;
    drvcmd[1] = (uint32_t) device;

    driver_exec(pciGetInfo(device) | DRIVER_TYPE_PCI, drvcmd);
    free(drvcmd);

    /* after all disk drivers have been initialized this one should be called */
    diskio_init();

    MBR_enumerate();
}

void main(void)
{
    unsigned int exit_code = 0;
    uint16_t *buf;

    exit_code = screen_basic_init();

    if(exit_code != EXIT_CODE_GLOBAL_SUCCESS)
        while(1);

    init_env();

    buf = malloc(2048);

    READ(2, 0, 1, buf);
    trace("buffer: 0x%x\n", buf);

#ifndef NO_DEBUG_INFO /* you can define NO_DEBUG_INFO in types.h and it'll make all modules quiet */
    print((char*) "[KERNEL] ");
    info_print_version();
    print((char*)"\n");
#endif

    while(1);
}
