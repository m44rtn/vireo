/*
MIT license
Copyright (c) 2021 Maarten Vermeulen

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

#include "../include/kernel.h"

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


char *kernel_get_version_str(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_VERSION_STR};
    asm_syscall(&hdr);

    return (char *) hdr.response_ptr;
}

kernel_ver_t *kernel_get_version_number(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_VERSION_NUM};
    asm_syscall(&hdr);

    return (kernel_ver_t *) hdr.response_ptr;
}

uint8_t *kernel_get_free_interrupt_handlers(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_FREE_INT_HANDLERS};
    asm_syscall(&hdr);

    return (uint8_t *) hdr.response_ptr;
}

err_t kernel_add_interrupt_handler(void *_handler, uint8_t _int)
{
    int_request_t req = {
        .hdr.system_call = SYSCALL_ADD_INT_HANDLER,
        .handler = _handler,
        .intr = _int
    };

    asm_syscall(&req);

    return req.hdr.exit_code;
}

uint32_t kernel_get_systicks(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_SYSTICKS};
    asm_syscall(&hdr);

    // in this case, the pointer is not a pointer but a value
    return (uint32_t) hdr.response_ptr;
}

void kernel_sleep(uint32_t _ms)
{
    sleep_request_t req = {
        .hdr.system_call = SYSCALL_SLEEP,
        .ms = _ms
    };

    asm_syscall(&req);
}
