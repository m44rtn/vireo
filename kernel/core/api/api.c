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
#include "syscalls.h"

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
#include "../exec/task.h"
#include "../exec/exec.h"
#include "../util/util.h"

#define API_LAST_SEGMENT            0xFF
#define API_SYSCALL_SEGMENT_SIZE    0x100u

#define API_LAST_RESERVED       API_SEG_DEBUG
#define API_LAST_RESERVED_SEGM  (0x0C00u / API_SYSCALL_SEGMENT_SIZE)

#define API_SEG_KERNEL          0x0000 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_SCREEN          0x0100 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_MEMORY          0x0200 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_DISK_ABS        0x0300 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_FILESYSTEM      0x0400 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_PROGRAM         0x0500 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_DRIVER          0x0A00 / API_SYSCALL_SEGMENT_SIZE
#define API_SEG_API             0x0B00 / API_SYSCALL_SEGMENT_SIZE

#define API_SEG_DEBUG           0x0C00 / API_SYSCALL_SEGMENT_SIZE

// 255 * sizeof(api_space_t) = 4080 bytes
typedef struct api_spaces_t
{
    char filename[12]; // filename of the program using the api_space
    uint32_t api_handler;
} __attribute__((packed)) api_spaces_t;

// ==== for API calls
typedef struct api_listing_t
{
    char filename[12];
    api_space_t start_syscall_space; // e.g., 0xff00 (which would run until 0xffff)
} __attribute__((packed)) api_listing_t;

typedef struct api_request_t
{
    syscall_hdr_t hdr;
    uint32_t handler;
    api_space_t space;
} __attribute__((packed)) api_request_t;

// === end for API calls

api_spaces_t *api_spaces = NULL;

void api_init(void)
{
    api_spaces = evalloc(API_LAST_RESERVED_SEGM * sizeof(api_spaces_t), PID_KERNEL);

    for(uint16_t i = 0; i < API_LAST_RESERVED_SEGM + 1; ++i)
        memcpy(&api_spaces[i].filename[0], (char *) "VIREO.SYS", strlen("VIREO.SYS"));
    
}

void *api_dispatcher(void *eip, void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *)req;
    response_hdr_t *out = (response_hdr_t *)req;

    if(!api_spaces)
        api_init();

    uint8_t spaces_index = (uint8_t) ((hdr->system_call & 0xFF00) / API_SYSCALL_SEGMENT_SIZE);
    
    if(api_spaces[spaces_index].filename[0] == 0)
    {
        out->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        out->size = sizeof(response_hdr_t);
        return eip;
    }

    switch(hdr->system_call / API_SYSCALL_SEGMENT_SIZE)
    {
        default:
            EXEC_CALL_FUNC((uint32_t *) api_spaces[spaces_index].api_handler, req); 
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
            // allows a program to get more information about drivers
            // and register/unregister drivers dynamically
            driver_api(req);
        break;

        case API_SEG_API:
            // allows a program to get aou listings and request their
            // own api space for syscalls (useful for e.g., drivers)
            api_api(req);
        break;

        case API_SEG_DEBUG:
            debug_api(req);
        break;
    }

    return (void *) eip;
}

api_space_t api_get_free_space(void)
{
    for(uint16_t i = 0; i < API_LAST_SEGMENT; ++i)
        if(api_spaces[i].filename[0] == 0)
            return i;

    return (api_space_t) MAX;
}

api_space_t api_free_space_request(uint32_t handler)
{
    api_space_t space = api_get_free_space();
            
    if(space == (api_space_t) MAX)
        return space;
    
    api_spaces[space].api_handler = handler;

    const char *file = prog_get_filename(prog_get_current_running());
            
    // find the filename in the path
    uint32_t i = find_in_str((char *) &file[strlen(file) - 12], "/");
    i = (i == MAX) ? strlen(file) - 12 : i + strlen(file) - 12;
    
    // copy filename to api_spaces.filename
    memcpy(&api_spaces[space].filename[0], (void *) &file[i + 1], strlen(&file[i + 1]));

    return (api_space_t) (space * API_SYSCALL_SEGMENT_SIZE);
}

void api_api(void *req)
{

    syscall_hdr_t *hdr = req;

    switch(hdr->system_call)
    {
        case SYSCALL_GET_API_LISTING:
        {
            api_listing_t *list = evalloc(API_LAST_RESERVED_SEGM * sizeof(api_spaces_t), prog_get_current_running());
            
            for(uint16_t i = 0; i < API_LAST_SEGMENT; ++i)
            {
                memcpy(&list[i].filename[0], &api_spaces[i].filename[0], 11);
                list[i].start_syscall_space = (api_space_t) (i * API_SYSCALL_SEGMENT_SIZE);
            }
            break;
        }

        case SYSCALL_REQUEST_API_SPACE:
        {
            api_request_t *a = req;
            a->hdr.response = api_free_space_request(a->handler);
            break;
        }

        case SYSCALL_FREE_API_SPACE:
        {
            api_request_t *a = req;
            uint8_t space = (uint8_t) ((a->space & 0xFF00) / API_SYSCALL_SEGMENT_SIZE);

            if(space <= API_LAST_RESERVED_SEGM)
                break;
            
            api_spaces[space].filename[0] = 0;
            break;
        }

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}
