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

#define PS2KEYB_DRIVER_ID   DRIVER_TYPE_HID | KEYB_INT

#define PS2KEYB_CALL_REGISTER_SUBSCRIBER     0x00
/* parameters:
    * [uint32_t] (param 0) program or driver id
    * [void *] (param 1) pointer to outbuffer
    * [size_t] (param 2) size of outbuffer
    * [uin32_t] (param 3) reset type --> see above
    
    returns: (syscall_hdr_t.response) buffer id
*/ 

#define PS2KEYB_CALL_DEREGISTER_SUBSCRIBER   0x01
/* parameters:
    * [uint32_t] (param 0) program or driver id
    * [uint32_t] (param 1) buffer id 
    
    if the buffer id provided was not requested by the program
    of the provided program/driver id
    the request to deregister the outbuffer will be ignored.
*/

#define PS2KEYB_CALL_LAST_KEY            0x02
/* no parameters

    returns: (syscall_hdr_t.response) last key pressed by user
*/

#endif

