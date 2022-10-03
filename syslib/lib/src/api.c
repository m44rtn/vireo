/*
MIT license
Copyright (c) 2022 Maarten Vermeulen

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

#include "../include/api.h"

typedef struct api_request_t
{
    syscall_hdr_t hdr;
    function_t handler;
    api_space_t space;
} __attribute__((packed)) api_request_t;

api_listing_t *api_get_syscall_listing(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_API_LISTING};
    PERFORM_SYSCALL(&hdr);
    
    return (api_listing_t *) hdr.response_ptr;
}

api_space_t api_get_api_space(function_t _handler)
{
    api_request_t req = {.hdr.system_call = SYSCALL_REQUEST_API_SPACE, .handler = _handler};
    PERFORM_SYSCALL(&req);
    
    return (api_space_t) req.hdr.response;
}

void api_free_api_space(api_space_t _space)
{
    api_request_t req = {.hdr.system_call = SYSCALL_FREE_API_SPACE, .space = _space};
    PERFORM_SYSCALL(&req);
}

