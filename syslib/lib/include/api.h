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

#ifndef __API_H__
#define __API_H__

#include "types.h"
#include "call.h"

#ifndef PERFORM_SYSCALL
    #define PERFORM_SYSCALL(req)    __asm__ __volatile__("mov %0, %%esi\n\t" "int $0x80" :: "rm" (req));
#endif

typedef struct api_listing_t
{
    char filename[11];
    api_space_t start_syscall_space; // e.g., 0xff00 (which would run until 0xffff)
} __attribute__((packed)) api_listing_t;



// get all non-kernel api syscalls, returns api_listing_t * 
api_listing_t *api_get_syscall_listing(void);

// request api space, returns api_space_response_t containing:
// - a 16-bit number with the starting 'address' of the api space you can define up to 
//   [starting_address + 0x0ff] syscalls using this address (for example 0x0f00-0x0fff).
// - a key which you can use to free your api space.
api_space_t api_get_api_space(function_t _handler);

// free api space.
void api_free_api_space(api_space_t _space);



#endif
