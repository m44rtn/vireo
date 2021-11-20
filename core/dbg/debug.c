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

#include "debug.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../screen/screen_basic.h"

#include "../api/api.h"
#include "../api/syscalls.h"

void debug_print_warning(const char *warning)
{
    screen_basic_set_screen_color(0x0E);
    print_value( "[WARNING] %s\n\n", (uint32_t) warning);
    screen_basic_set_screen_color(0x07);
}

void debug_print_error(const char *error)
{
    screen_basic_set_screen_color(0x04);
    print_value( "[ERROR] %s\n\n", (uint32_t) error);
    screen_basic_set_screen_color(0x07);
}

void debug_api(void *req)
{
    syscall_hdr_t *hdr = req;

    switch(hdr->system_call)
    {
        case SYSCALL_NOP:
            // general fail exit code to indicate the API works and that the kernel
            // is (still) alive.
            hdr->exit_code = EXIT_CODE_GLOBAL_GENERAL_FAIL;    
        break;

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}
