/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

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

#include "panic.h"
#include "info.h"

#include "../include/types.h"

#include "../cpu/cpu.h"

#include "../screen/screen_basic.h"

/* panic with CPU state */
void panic(const char *type, const char *error)
{
    CPU_STATE cpustate = CPU_get_state();
    screen_set_hexdigits(8);

    print((char *)"--- kernel panic ---\n");
    trace((char *)"Fatal %s: ", (uint32_t) type);
    trace((char *)"%s\n", (uint32_t) error);
    
    /* kernel version string */
    print((char *)"Kernel version string: ");
    info_print_version();

    print((char *)"\nRegister dump:\n\t");
    trace((char *)"eax=0x%x ", cpustate.eax);
    trace((char *)"ecx=0x%x ", cpustate.ecx);
    trace((char *)"edx=0x%x\n\t", cpustate.edx);

    trace((char *)"ebx=0x%x ", cpustate.ebx);
    trace((char *)"esp=0x%x ", cpustate.esp);
    trace((char *)"ebp=0x%x\n\t", cpustate.ebp);

    trace((char *)"esi=0x%x ", cpustate.esi);
    trace((char *)"edi=0x%x\n\n", cpustate.edi);

    trace((char *)"\teip=0x%x\n", (unsigned int) cpustate.eip);

    print((char *)"--- end kernel panic ---\n");

    screen_basic_disable_cursor();
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);

    while(1);
}

/* panic without cpu state --> mainly used for non-cpu-exceptions */
void easy_panic(const char *type, const char *error)
{
    print((char *)"--- kernel panic ---\n");
    trace((char *)"Fatal %s: ", (uint32_t) type);
    trace((char *)"%s\n", (uint32_t) error);
    
    /* kernel version string */
    print((char *)"Kernel version string: ");
    info_print_version();

    print((char *)"--- end kernel panic ---\n");

    screen_basic_disable_cursor();
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);

    while(1);
}