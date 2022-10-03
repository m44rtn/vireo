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

#include "../include/driver.h"

typedef struct driver_call_t
{
    syscall_hdr_t hdr;
    driver_t type;
    char *path;
} __attribute__((packed)) driver_call_t;

driver_info_t *driver_get_list(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_DRIVER_GET_LIST};
    PERFORM_SYSCALL(&hdr);

    return (driver_info_t *) hdr.response_ptr;
}

err_t driver_add(const char *_path, driver_t _type)
{
    driver_call_t req = {
        .hdr.system_call = SYSCALL_DRIVER_ADD,
        .type = _type,
        .path = (char *) (_path)
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}

err_t driver_remove(driver_t _type)
{
    driver_call_t req = {
        .hdr.system_call = SYSCALL_DRIVER_REMOVE,
        .type = _type,
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}
