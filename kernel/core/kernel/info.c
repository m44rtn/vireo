/*
MIT license
Copyright (c) 2019-2022 Maarten Vermeulen

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

#include "info.h"

#include "../include/types.h"

#include "../screen/screen_basic.h"
#include "../util/util.h"
#include "../memory/memory.h"

// Version str format: Vireo-[version] build [BUILD_NUMBER] (v[MAJOR].[MINOR][REVISION])
#define INFO_VER_STR_START  "Vireo-II build "

void info_print_version(void)
{
    print_value(INFO_VER_STR_START "%i (", BUILD);
    print_value("v%i.", MAJOR);
    print_value("%i", MINOR);
    print_value("%s)\n", (uint32_t) REV);
}

void info_print_full_version(void)
{
    print((char*) "\n[KERNEL] ");
    info_print_version();
    print_value("[KERNEL] Build on: %s ", (uint32_t) BUILDDATE);
    print_value("at %s\n", (uint32_t) BUILDTIME);
}

void info_print_panic_version(void)
{
    print(" Kernel version string: ");
    info_print_version();
    print_value(" Build on: %s ", (uint32_t) BUILDDATE);
    print_value("at %s\n", (uint32_t) BUILDTIME);
}

char *info_make_version_str(void)
{
    char *str = kmalloc(512);
    memset(str, 512, 0); // FIXME: buffer is filled with 0x20 if memset is not used
    str_add_val(str, INFO_VER_STR_START "%i (", BUILD);

    uint32_t i = strlen(str);

    str_add_val(&str[i], "v%i.", MAJOR);
    i = strlen(str);

    str_add_val(&str[i], "%i", MINOR);
    i = strlen(str);

    str_add_val(&str[i], "%s)", (uint32_t) REV);
    
    return str;
}
