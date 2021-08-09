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
#include "info.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../util/util.h"

#include "../memory/memory.h"
#include "../memory/paging.h"

#include "../cpu/interrupts/isr.h"

#include "../hardware/timer.h"

#define KERNEL_REV_MAX_SIZE     8

typedef struct int_request_t
{
    syscall_hdr_t hdr;
    void *handler;
    uint8_t intr;
} __attribute__((packed)) int_request_t;

typedef struct sleep_request_t
{
    syscall_hdr_t hdr;
    uint32_t ms;
} __attribute__((packed)) sleep_request_t;

typedef struct kernel_ver_t
{
    uint16_t major;
    uint16_t minor;
    uint32_t build;
    char rev[KERNEL_REV_MAX_SIZE];
} __attribute__((packed)) kernel_ver_t;

void kernel_api_handler(void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *) req;

    switch(hdr->system_call)
    {
        case SYSCALL_VERSION_STR:
        {
            // full version string
            char *str = info_make_version_str();
            
            hdr->response_ptr = api_alloc(strlen(str) + 1);
            hdr->response_size = strlen(str) + 1;
            memcpy((hdr->response_ptr), str, strlen(str) + 1);
            kfree(str);
            break;
        }

        case SYSCALL_VERSION_NUM:
        {
            // version number (build, major, minor, rev)
            kernel_ver_t *ver = api_alloc(sizeof(kernel_ver_t));
            hdr->response_ptr = ver;
            hdr->response_size = sizeof(kernel_ver_t);

            ver->build = BUILD;
            ver->major = MAJOR;
            ver->minor = MINOR;

            memcpy(&(ver->rev), REV, strlen(REV));
            break;
        }

        case SYSCALL_FREE_INT_HANDLERS:
            hdr->response_ptr = api_alloc(MAX_PIC_INTERRUPTS * sizeof(void *));
            hdr->response_size = MAX_PIC_INTERRUPTS * sizeof(void *);
            memcpy(hdr->response_ptr, isr_get_extern_handlers(), sizeof(void *));
        break;

        case SYSCALL_ADD_INT_HANDLER:
        {
            int_request_t *intr = (int_request_t *) req;
            isr_set_extern_handler(intr->intr, intr->handler);
            break;
        }

          case SYSCALL_GET_SYSTICKS:
            hdr->response_ptr = (void *) timer_getCurrentTick();
        break;
        
        case SYSCALL_SLEEP:
        {
            sleep_request_t *r = (sleep_request_t *) req;
            sleep(r->ms);    
            break;
        }

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;

    }
}
