/*
MIT license
Copyright (c) 2023 Maarten Vermeulen

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

#ifndef __CP_H__
#define __CP_H__

#include "types.h"
#include "call.h"

typedef struct cp_api_req
{
    syscall_hdr_t hdr;
    char *param;
} __attribute__((packed)) cp_api_req;

#define CP_GET_VERSION          0x00
/* 
    returns a string containing the CP version in cp_api_req.hdr.response_ptr
*/ 

#define CP_GET_CWD              0x01
/* 
    returns the path to the current working directory in cp_api_req.hdr.response_ptr
*/

#define CP_SET_CWD              0x02
/* 
    sets the current working directory to the contents of cp_api_req.param    
*/

#define CP_EXEC_CMD             0x03
/* 
    executes the command given in cp_api_req.param as if it were typed by the user
*/

#define CP_GET_ERRLVL           0x04
/* 
    returns the error level (exit code) of the previous binary in cp_api_req.hdr.response  
*/

#endif // __CP_H__