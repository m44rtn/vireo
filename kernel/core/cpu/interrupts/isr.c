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
#include "IDT.h"

#include "../../hardware/pic.h"
#include "../../hardware/timer.h"

#include "../../include/types.h"
#include "../../include/exit_code.h"

#include "../../memory/memory.h"
#include "../../screen/screen_basic.h"

#include "../../io/io.h"

#include "../../kernel/panic.h"
#include "../../kernel/kernel.h"
#include "../../exec/exec.h"

#include "../../util/util.h"

#define ISR_EXT_HANDLER_TABLE_SIZE      1024 // bytes
#define ISR_EXT_HANDLER_TABLE_ENTRIES   ISR_EXT_HANDLER_TABLE_SIZE / sizeof(extern_isr_handlers_t)
#define ISR_FIRST_ALLOWED_EXT_INT       0x20U

typedef struct extern_isr_handlers_t
{
    uint8_t intr;
    void *handler;
} __attribute__((packed)) extern_isr_handlers_t;

extern_isr_handlers_t *g_ext_isr_handlers = NULL;

void isr_api_init(void)
{
    g_ext_isr_handlers = kmalloc(ISR_EXT_HANDLER_TABLE_SIZE);
    memset(g_ext_isr_handlers, ISR_EXT_HANDLER_TABLE_SIZE, 0);
}

static uint32_t isr_get_free_ext_index(void)
{
    for(uint32_t i = 0; i < ISR_EXT_HANDLER_TABLE_SIZE; i++)
        if(g_ext_isr_handlers[i].handler == 0)
            return i;
    
    return MAX;
}

err_t isr_set_extern_handler(uint8_t type, void *handler)
{    
    if(g_ext_isr_handlers == NULL)
        return EXIT_CODE_GLOBAL_NOT_INITIALIZED;
    else if(type < ISR_FIRST_ALLOWED_EXT_INT)
        return EXIT_CODE_GLOBAL_OUT_OF_RANGE;
    else if(handler == NULL || handler < memory_get_kernel_space_end())
        return EXIT_CODE_GLOBAL_INVALID;

    uint32_t index = isr_get_free_ext_index();

    if(index == MAX)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    
    g_ext_isr_handlers[index].intr = type;
    g_ext_isr_handlers[index].handler = handler;
    
    if(!idt_handler_in_use(type) && type < PIC_LAST_INTERRUPT_VECTOR)
        IDT_add_handler(type, (uint32_t) ISR_STANDARD); 

    if(type < PIC_LAST_INTERRUPT_VECTOR)
        return EXIT_CODE_GLOBAL_SUCCESS;
    
    // if we get here, the interrupt type is a software interrupt. in this case,
    // we can't use the standard handler because we don't know the interrupt
    // number on interrupt.
    if(!idt_handler_in_use(type))
        IDT_add_handler(type, (uint32_t) handler); 

    return EXIT_CODE_GLOBAL_SUCCESS;    
}

uint32_t isr_get_extern_handlers(uint8_t type, void **o_extern_handlers, size_t ext_handlers_buffer_size)
{
    if(g_ext_isr_handlers == NULL)
        return MAX;
    
    // safety so that the user gives a buffer that's big enough to hold all entries
    if(ext_handlers_buffer_size != (ISR_EXT_HANDLER_TABLE_ENTRIES * sizeof(void *)))
        return MAX;
    
    uint32_t j = 0;
    for(uint32_t i = 0; i < ISR_EXT_HANDLER_TABLE_ENTRIES; i++)
    {
        if(g_ext_isr_handlers[i].intr != type || g_ext_isr_handlers[i].handler == NULL)
            continue;
        
        o_extern_handlers[j] = g_ext_isr_handlers[i].handler;
        j++;
    }

    return j;
}

void isr_delete_extern_handler(uint8_t type, void * handler)
{
    uint32_t i = 0;

    for(; i < ISR_EXT_HANDLER_TABLE_ENTRIES; i++)
        if(g_ext_isr_handlers[i].intr == type && g_ext_isr_handlers[i].handler == handler)
            break;
    
    if(i >= ISR_EXT_HANDLER_TABLE_ENTRIES)
        return;

    memset(&g_ext_isr_handlers[i], sizeof(extern_isr_handlers_t), 0);

    if(type > PIC_LAST_INTERRUPT_VECTOR)
        idt_remove_handler(type);
}

void isr_delete_extern_handlers_in_range(void *from, void *to)
{
    for(uint32_t i = 0; i < ISR_EXT_HANDLER_TABLE_ENTRIES; i++)
    {
        if(g_ext_isr_handlers[i].handler < from || g_ext_isr_handlers[i].handler > to)
            continue;
        
        isr_delete_extern_handler(g_ext_isr_handlers[i].intr, g_ext_isr_handlers[i].handler);        
    }
}

uint32_t isr_max_extern_handlers(void)
{
    return ISR_EXT_HANDLER_TABLE_ENTRIES;
}

static void isr_exec_extern(uint8_t type)
{
    // NOTE: this makes ISRs long and relatively slow which is not ideal.
    //       on the other hand, interrupts are enabled during extern execution (unless user disables them again)
    //       and the PIC_EOI has been given before all this is called.
    void **extern_handlers = kmalloc(ISR_EXT_HANDLER_TABLE_ENTRIES * sizeof(void *));

    if(!extern_handlers)
        return;

    uint32_t n_entries = isr_get_extern_handlers(type, extern_handlers, ISR_EXT_HANDLER_TABLE_ENTRIES * sizeof(void *));

    for(uint32_t i = 0; i < n_entries; i++)
        asm_exec_isr(extern_handlers[i]);

    kfree(extern_handlers);
}

void ISR_STANDARD_HANDLER(void)
{
    uint8_t pic = nth_bit(PIC_read_ISR(), sizeof(uint16_t) * 8);

    if(pic == (uint8_t) MAX)
        return;
    
    PIC_EOI(pic);

    isr_exec_extern((uint8_t) (pic + ISR_FIRST_ALLOWED_EXT_INT));
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
        break;
    }
    
}

void ISR_20_HANDLER(void)
{
    timer_incTicks();
    PIC_EOI(0);

    if(g_kernel_flags & KERNEL_FLAG_HAS_INIT)
        isr_exec_extern(0x20);
}
