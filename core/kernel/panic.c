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

#include "panic.h"
#include "info.h"

#include "../include/types.h"

#include "../cpu/cpu.h"

#include "../screen/screen_basic.h"

// FIXME: make this nice some day

/* panic with CPU state */
void panic(const char *type, const char *error)
{
    CPU_STATE cpustate = CPU_get_state();
    screen_set_hexdigits(8);

    print("\n [KERNEL PANIC]\n");
    print_value(" Fatal %s: ", (uint32_t) type);
    print_value("%s\n", (uint32_t) error);
    
    /* kernel version string */
    print(" Kernel version string: ");
    info_print_version();

    print("\n Register dump:\n\t");
    print_value("eax: 0x%x\t", cpustate.eax);
    print_value("ecx: 0x%x\t", cpustate.ecx);
    print_value("edx: 0x%x\n\t", cpustate.edx);

    print_value("ebx: 0x%x\t", cpustate.ebx);
    print_value("esp: 0x%x\t", cpustate.esp);
    print_value("ebp: 0x%x\n\t", cpustate.ebp);

    print_value("esi: 0x%x\t", cpustate.esi);
    print_value("edi: 0x%x\n\n", cpustate.edi);

    print_value("\teip: 0x%x\n", (unsigned int) cpustate.eip);

    screen_basic_disable_cursor();
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);

    while(1);
}

/* panic without cpu state --> mainly used for non-cpu-exceptions */
void easy_panic(const char *type, const char *error, const char *file, const uint32_t line, uint32_t fptr)
{
    print("\n [KERNEL PANIC]\n");
    print_value(" Fatal %s: ", (uint32_t) type);
    print_value("%s\n", (uint32_t) error);
    
    /* kernel version string */
    print(" Kernel version string: ");
    info_print_version();

    print("\n Debug information:\n\t");
    print_value("file: %s\n\t", (uint32_t) file);
    print_value("line: %i\n\t", line);
    print_value("fptr: 0x%x\n", (uint32_t) fptr);

    screen_basic_disable_cursor();
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);

    while(1);
}

void really_easy_panic(const char *type, const char *error)
{
    print("\n [KERNEL PANIC]\n");
    print_value(" Fatal %s: ", (uint32_t) type);
    print_value("%s\n", (uint32_t) error);
    
    /* kernel version string */
    print(" Kernel version string: ");
    info_print_version();

    screen_basic_disable_cursor();
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);

    while(1);
}