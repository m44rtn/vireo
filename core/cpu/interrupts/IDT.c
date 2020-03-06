/*
MIT license
Copyright (c) 2019 Maarten Vermeulen

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

#include "IDT.h"

#include "../../include/types.h"

typedef struct IDT_ENTRY
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t always_zero;
    uint8_t type_attr;
    uint16_t offset_hi;
} __attribute__((packed)) IDT_ENTRY;

typedef struct IDT_DESCRIPTOR
{
    uint16_t size;
    uint32_t address;
} __attribute__((packed)) IDT_DESCRIPTOR;

IDT_DESCRIPTOR IDT_desc;
IDT_ENTRY IDT[256];

static void IDT_default_list(void);


/* creates a basic IDT with the following stuff:
        - DIVISON_BY_ZERO
        - NON_MASKABLE_INT
        - DEBUG
        - OVERFLOW
        - BOUNDS_CHECK
        - INVALID_OPCODE
        - COPROCESSOR_NOT_AVAILABLE
        - DOUBLE_FAULT
        - COPROCESSOR_SEGMENT_OVERRUN
        - INVALID_TSS
        - SEGMENT_NOT_PRESENT
        - STACK_FAULT
        - GENERAL_PROTECTION_FAULT
        - PAGE_FAULT
        - COPROCESSOR_ERROR

        - TIMER     (does little at this moment)
        - KEYBOARD (directly prints to screen and doesn't store)
        
    Some exceptions hang, some panic and others will display a message
         */
        
void IDT_setup(void)
{
    IDT_default_list();

    IDT_desc.size    = (sizeof(IDT_ENTRY) * 256) - 1;
    IDT_desc.address = (uint32_t) &IDT;
    
    ASM_IDT_SUBMIT((uint32_t *) &IDT_desc);    
}

void IDT_add_handler(uint8_t index, uint32_t handler)
{
    IDT[index].offset_low   =  ((uint16_t) handler & 0xFFFF);
    IDT[index].offset_hi    =  ((uint16_t)(handler >> 16)) & 0xFFFF;

    IDT[index].selector     = 0x08;
    IDT[index].always_zero  = 0;

    /* present, DPL = 00, s = 0 and gatetype = 0xE */
    IDT[index].type_attr    = 0x8E;
}

void IDT_reset(void)
{
    ASM_IDT_SUBMIT((uint32_t *) &IDT_desc);   
}

static void IDT_default_list(void)
{
    IDT_add_handler(0x00, (uint32_t) ISR_00);
    IDT_add_handler(0x01, (uint32_t) ISR_01);
    IDT_add_handler(0x02, (uint32_t) ISR_02);
    IDT_add_handler(0x03, (uint32_t) ISR_03);
    IDT_add_handler(0x04, (uint32_t) ISR_04);
    IDT_add_handler(0x05, (uint32_t) ISR_05);
    IDT_add_handler(0x06, (uint32_t) ISR_06);
    IDT_add_handler(0x07, (uint32_t) ISR_07);
    IDT_add_handler(0x08, (uint32_t) ISR_08);
    IDT_add_handler(0x09, (uint32_t) ISR_09);
    IDT_add_handler(0x0A, (uint32_t) ISR_0A);
    IDT_add_handler(0x0B, (uint32_t) ISR_0B);
    IDT_add_handler(0x0C, (uint32_t) ISR_0C);
    IDT_add_handler(0x0D, (uint32_t) ISR_0D);
    IDT_add_handler(0x0E, (uint32_t) ISR_0E);
    
    IDT_add_handler(0x20, (uint32_t) ISR_20);   
    IDT_add_handler(0x21, (uint32_t) ISR_21);    
}
