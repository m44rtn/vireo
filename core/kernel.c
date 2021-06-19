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
#include "hardware/pic.h"
#include "hardware/driver.h"

#include "dbg/dbg.h"

#include "dsk/diskio.h"
#include "dsk/mbr.h"
#include "dsk/cd.h"

#include "exec/exec.h"
#include "exec/flat.h"

#include "kernel/panic.h"
#include "kernel/info.h"

/* TODO: remove */
#include "drv/COMMANDS.H"
#include "drv/IDE_commands.h"
#include "drv/FS_commands.h"
#include "drv/FS_TYPES.H"

#include "drv/FS/fat.h"
#include "drv/FS/fs_exitcode.h"

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
    kfree(devicelist);

    // initialize IDE driver
    drvcmd = kmalloc(DRIVER_COMMAND_PACKET_LEN * sizeof(uint32_t *));
    drvcmd[0] = DRV_COMMAND_INIT;
    drvcmd[1] = (uint32_t) device;

    driver_exec(pciGetInfo(device) | DRIVER_TYPE_PCI, drvcmd);
    kfree(drvcmd);

    /* after all disk drivers have been initialized this one should be called */
    diskio_init();

    MBR_enumerate();
    cd_init();

    // TODO: get a function to do this
    // ----------> One way would be to let MBR handle this stuff
    // driver_addInternalDriver((0x0B | DRIVER_TYPE_FS));
    // drv[0] = DRV_COMMAND_INIT;
    // drv[1] = 0;
    // drv[2] = 0;
    // drv[3] = 0x0B;
    // driver_exec((0x0B | DRIVER_TYPE_FS), drv);
}

void main(void)
{
    unsigned int exit_code = 0;
    uint32_t *drv = kmalloc(5 * sizeof(uint32_t));

    exit_code = screen_basic_init();

    if(exit_code != EXIT_CODE_GLOBAL_SUCCESS)
        while(1);

    init_env();

    drv[0] = FS_COMMAND_READ;
    drv[1] = (uint32_t) "CD0/TEST/TEST.BIN\0";
    driver_exec((FS_TYPE_ISO | DRIVER_TYPE_FS), drv);
    print_value("READ FILE WITH ERROR CODE: %x\n", drv[4]);
    print_value("buffer location: 0x%x\t", drv[2]);
    print_value("size: %i\n", drv[3]);
    print_value("file that should have been read: %s\n", drv[1]);

    if(drv[4] == EXIT_CODE_FS_UNSUPPORTED_DRIVE)
        print("Error: drive specification unsupported\n");
    
#ifndef NO_DEBUG_INFO /* you can define NO_DEBUG_INFO in types.h and it'll make all modules quiet */
    info_print_full_version();    
    print((char*)"\n");
#endif

    flat_call_binary((void *) drv[2], 512);
    /*conways_game_of_life();*/

    while(1);
}
