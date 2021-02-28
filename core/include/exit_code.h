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

#ifndef __EXIT_CODE_H__
#define __EXIT_CODE_H__

#define FIRST_NOT_RESV_EXIT_CODE            0x10

#define EXIT_CODE_GLOBAL_SUCCESS            0x00
#define EXIT_CODE_GLOBAL_GENERAL_FAIL       0x01
#define EXIT_CODE_GLOBAL_NOT_IMPLEMENTED    0x02
#define EXIT_CODE_GLOBAL_OUT_OF_RANGE       0x03

#define EXIT_CODE_GLOBAL_UNSUPPORTED        0x0A
#define EXIT_CODE_OUT_OF_MEMORY             0x0B
/* 0x00 - 0x0F are reserved for global exit_codes used by the kernel*/

#endif