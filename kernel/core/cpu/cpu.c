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

#include "cpu.h"

#include "../include/types.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

extern const uint8_t CPUID_AVAILABLE;
extern const char *CPUID_VENDOR_STRING;
extern const char *CPUID_CPUNAME_STRING;

CPU_STATE state;

void CPU_init(void)
{
    ASM_CHECK_CPUID();
    
    if(!CPUID_AVAILABLE) 
        return;
    
    ASM_CPU_GETVENDOR();

    #ifndef NO_DEBUG_INFO
    print_value( "[CPU] %s\n", (unsigned int) CPUID_VENDOR_STRING);
    #endif

    ASM_CPU_GETNAME();

    #ifndef NO_DEBUG_INFO
    print_value( "[CPU] %s\n\n", (unsigned int) CPUID_CPUNAME_STRING);
    #endif

}

CPU_STATE CPU_get_state(void)
{
    return state;
}

