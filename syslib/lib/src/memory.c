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

#include "../include/memory.h"

typedef struct valloc_t
{
    syscall_hdr_t hdr;
    size_t size;
} __attribute__((packed)) valloc_t;

typedef struct vfree_t
{
    syscall_hdr_t hdr;
    void *ptr;
} __attribute__((packed)) vfree_t;

memory_info_t *memory_get_info(err_t *err)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_MEM_INFO};
    PERFORM_SYSCALL(&hdr);

    *err = hdr.exit_code;

    return (memory_info_t *) hdr.response_ptr;
}

void *valloc(size_t _size)
{
    valloc_t req = {
        .hdr.system_call = SYSCALL_VALLOC,
        .size = _size
    };

    PERFORM_SYSCALL(&req);
    
    return req.hdr.response_ptr;
}

void vfree(void *_ptr)
{
    vfree_t req = {
        .hdr.system_call = SYSCALL_VFREE,
        .ptr = _ptr
    };

    PERFORM_SYSCALL(&req);
}
