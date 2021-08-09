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

#include "isr.h"

#include "../../hardware/pic.h"
#include "../../hardware/timer.h"

#include "../../include/types.h"

#include "../../screen/screen_basic.h"

#include "../../io/io.h"

#include "../../kernel/panic.h"
#include "../../exec/exec.h"

#include "../../util/util.h"

void *extern_handlers[MAX_PIC_INTERRUPTS];

void isr_set_extern_handler(uint8_t type, void *handler)
{
    if(type >= MAX_PIC_INTERRUPTS)
        return;

    extern_handlers[type] = handler;
}

void **isr_get_extern_handlers(void)
{
    return &extern_handlers;
}

void ISR_STANDARD_HANDLER(void)
{
    uint16_t pic = nth_bit(PIC_read_ISR() & ~PIC_CASCADE, sizeof(uint16_t) << 3);
    
    // FIXME: when user mode exists, isr should be executed there
    if((pic && pic < 0xFF) && extern_handlers[pic] != NULL)
        asm_exec_isr(extern_handlers[pic]);
}

void ISR_00_HANDLER(void)
{
    panic(PANIC_TYPE_EXCEPTION, "DIVIDE_BY_ZERO");
}

void ISR_01_HANDLER(void)
{
    panic(PANIC_TYPE_EXCEPTION, "DEBUG");
}

void ISR_05_HANDLER(void)
{
    panic(PANIC_TYPE_EXCEPTION, "BOUND_RANGE_EXCEEDED");
}

void ISR_06_HANDLER(void)
{
    panic(PANIC_TYPE_EXCEPTION, "INVALID_OPCODE");
}

void ISR_0D_HANDLER(void)
{
    panic(PANIC_TYPE_EXCEPTION, "GENERAL_PROTECTION_FAULT");
}

void ISR_0E_handler(uint32_t error_code)
{
    /* only use the bottom three bits */
    error_code = error_code & 0x07;

    switch(error_code)
    {
        /* should I have used defines? maybe */

        /* error code for read or write to a non-present page from supervisor */ 
        case 0: 
        case 2:
            panic(PANIC_TYPE_EXCEPTION, "SUPERVISOR_NON_PRESENT_PAGE");
        break;

        /* error code for read or write to a page causing a page-fault, supervisor */
        case 1:
        case 3:
            panic(PANIC_TYPE_EXCEPTION, "SUPERVISOR_PAGE_FAULT");
        break;

        /* error code for read or write a non-present page from user */
        case 4:
        case 6:
            panic(PANIC_TYPE_EXCEPTION, "USER_NON_PRESENT_PAGE");
        break;

        /* error code for read or write to a page causing a page-fault, user */
        case 5:
        case 7:
            panic(PANIC_TYPE_EXCEPTION, "USER_PAGE_FAULT");
        break;

        default:
            panic(PANIC_TYPE_EXCEPTION, "PAGE_FAULT");
    }
    
}

void ISR_20_HANDLER(void)
{
    timer_incTicks();
    PIC_EOI(0);
}

void ISR_21_HANDLER(void)
{
    uint16_t character = (uint16_t) inb(0x60);
    
    /* to get rid of compiler warnings :)
      this'll be changed to something useful later on */
    if(character) { /* do something */ }
    
    PIC_EOI(1);
}