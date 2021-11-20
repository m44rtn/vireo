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

#include "api.h"

#include "../dbg/dbg.h"

#include "../include/exit_code.h"

#include "../memory/paging.h"

#include "../dsk/diskio.h"
#include "../memory/memory.h"
#include "../drv/FS_commands.h"
#include "../drv/FS_TYPES.H"
#include "../hardware/driver.h"

#include "../screen/screen_basic.h"
#include "../kernel/kernel.h"
#include "../dsk/fs.h"
#include "../exec/prog.h"

#define API_SYSCALL_SEGMENT     0x100

#define API_SEG_KERNEL          0x0000 / API_SYSCALL_SEGMENT
#define API_SEG_SCREEN          0x0100 / API_SYSCALL_SEGMENT
#define API_SEG_MEMORY          0x0200 / API_SYSCALL_SEGMENT
#define API_SEG_DISK_ABS        0x0300 / API_SYSCALL_SEGMENT
#define API_SEG_FILESYSTEM      0x0400 / API_SYSCALL_SEGMENT
#define API_SEG_PROGRAM         0x0500 / API_SYSCALL_SEGMENT
#define API_SEG_DRIVER          0x0A00 / API_SYSCALL_SEGMENT

#define API_SEG_DRIVERS_START   0x0B00 / API_SYSCALL_SEGMENT
#define API_SEG_DRIVERS_END     0xBFFF / API_SYSCALL_SEGMENT

#define API_SEG_DEBUG           0xFF00 / API_SYSCALL_SEGMENT

void api_dispatcher(void *eip, void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *)req;
    response_hdr_t *out = (response_hdr_t *)req;

    switch(hdr->system_call / API_SYSCALL_SEGMENT)
    {
        default:
            out->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
            out->size = sizeof(response_hdr_t);
        break;

        case API_SEG_KERNEL:
            kernel_api_handler(req);
        break;

        case API_SEG_SCREEN:
            // for as long as this kernel only supports screen_basic this will
            // be the only screen api handler
            screen_basic_api(req);
        break;

        case API_SEG_MEMORY:
            memory_api(req);
        break;

        case API_SEG_DISK_ABS:
            // allows a program to perform absolute disk operations
            diskio_api(req);
        break;

        case API_SEG_FILESYSTEM:
            // allows a program to use the file system
            fs_api(req);
        break;

        case API_SEG_PROGRAM:
            // allows a program to launch new (child) programs
            // and will keep track of them, is also responsible for terminating
            // programs
            prog_api(req);
        break;

        case API_SEG_DRIVER:
            // TODO
            // will allow a program to get more information about drivers
            // and register/unregister drivers dynamically
        break;

        case API_SEG_DEBUG:
            debug_api(req);
        break;
    }
}

void *api_alloc(size_t size, pid_t pid)
{
    uint8_t attr = (!pid) ? PAGE_REQ_ATTR_READ_WRITE | PAGE_REQ_ATTR_SUPERVISOR : 
                            PAGE_REQ_ATTR_READ_WRITE;
    PAGE_REQ req = {
        .pid = pid,
        .size = size,
        .attr = PAGE_REQ_ATTR_READ_WRITE
    };
    void *ptr = valloc(&req);

    dbg_assert(ptr);
    return ptr;
}