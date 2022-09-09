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

#include "main.h"

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
#include "exec/elf.h"
#include "exec/prog.h"

#include "kernel/panic.h"
#include "kernel/info.h"
#include "kernel/kernel.h"

#include "dsk/fs.h"
#include "screen/screen_basic.h"

/* TODO: remove */
#include "drv/COMMANDS.H"
#include "drv/IDE_commands.h"
#include "drv/FS_commands.h"
#include "drv/FS_TYPES.H"

#include "drv/FS/fat32.h"
#include "drv/FS/fs_exitcode.h"

#include "api/api.h"
#include "exec/task.h"

void loop(void)
{
    #ifndef NO_DEBUG_INFO
        print("[KERNEL] Looping!\n");
    #endif
    
    while(1);
}

/* initializes 'the environment' */
void init_env(void)
{
    /* GDT structures */
    GDT_ACCESS access;
    GDT_FLAGS flags;
    uint8_t exit_code;

    exit_code = screen_basic_init();

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

    /* after all disk drivers have been initialized this one should be called */
    diskio_init();

    MBR_enumerate();
    cd_init();

    prog_init();
    api_init();

}

void main(void)
{
    init_env();

#ifndef NO_DEBUG_INFO /* you can define NO_DEBUG_INFO in types.h and it'll make all modules quiet */
    info_print_full_version();    
    print((char*)"\n");
#endif    
    kernel_execute_config();

    loop();
}
