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

#ifndef __PS2KEYB_H__
#define __PS2KEYB_H__

#include "types.h"
#include "call.h"
#include "driver.h"

typedef struct ps2keyb_api_req
{
    syscall_hdr_t hdr;
    uint16_t *buffer;
    size_t buffer_size;
} __attribute__((packed)) ps2keyb_api_req;

#define PS2KEYB_DRIVER_ID   DRIVER_TYPE_HID | 0x21 // KEYB_INT

#define PS2KEYB_CALL_REGISTER_SUBSCRIBER     0x00
/* parameters:
    * [uint16_t *] pointer to buffer
    * [size_t] size of buffer
    
    requests with invalid values (i.e. size < sizeof(uint16_t) and buffer < kernel_space_border) will be ignored.
*/ 

#define PS2KEYB_CALL_DEREGISTER_SUBSCRIBER   0x01
/* parameters:
    * [uint16_t *] pointer to buffer
    * [size_t] size of buffer

    buffer and size are used to identify the right subscriber info within the driver.
    requests with invalid values (i.e. size < sizeof(uint16_t) and buffer < kernel_space_border) will be ignored.
*/

#define PS2KEYB_CALL_LAST_KEY            0x02
/* no parameters

    returns: (syscall_hdr_t.response) last key pressed by user
*/

#endif

