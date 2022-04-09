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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"
#include "call.h"

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "USER"
#endif

#ifndef NDEBUG
#include "screen.h"
#include "util.h"
#define assert(expression)                                              \
if(!(expression))                                                       \
    {                                                                   \
        screen_set_color(SCREEN_COLOR_BLACK | SCREEN_COLOR_YELLOW);     \
        screen_print( "[");                                             \
        screen_print(PROGRAM_NAME);                                     \
        screen_print( ":ASSERT] ");                                     \
        screen_print(__FILE__);                                         \
        screen_print( ": ");                                            \
        screen_print(intstr(__LINE__));                                 \
        screen_print( ": Assertion Failed\n");                          \
        screen_set_color(SCREEN_COLOR_DEFAULT);                         \
        while(1);                                                       \
    }                                                                   \
                                                       
#else
#define assert(ignore) (void) 0
#endif

// performs no operation, but will give an error code (GENERAL_FAIL) if succesful
err_t debug_nop(void);

#endif // __DEBUG_H__
