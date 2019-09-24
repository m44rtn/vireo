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
} __attribute((packed)) IDT_ENTRY;

IDT_ENTRY IDT[256];

/* creates a basic IDT with the following stuff:
        - DIVISON_BY_ZERO
        - NON_MASKABLE_INT
        - DEBUG
        - OVERFLOW
        - BOUNDS_CHECK
        - INVALID_OPCODE
        - COPROCESSOR_NOT_AVAILABLE
        - DOUBLE_FAULT
        - 
         */
void IDT_setup()
{
    
}