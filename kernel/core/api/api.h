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

#ifndef __API_H__
#define __API_H__

#include "../include/types.h"
#include "../exec/prog.h"

typedef struct
{
    uint16_t system_call;
    uint8_t exit_code;
    void *response_ptr;
    uint32_t response;
    size_t response_size;
} __attribute__((packed)) syscall_hdr_t;

typedef struct
{
    uint8_t exit_code;
    size_t size;
} __attribute__((packed)) response_hdr_t;

void api_init(void);
void *api_dispatcher(void *eip, void *req);
api_space_t api_get_free_space(void);
api_space_t api_handle_space_request(uint32_t handler);
void api_api(void *req);

extern void api_dispatcher_start(void);
extern void api_dispatcher_return(void *eip);

#endif // __API_H__
