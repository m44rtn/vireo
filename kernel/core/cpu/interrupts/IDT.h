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

#ifndef __IDT_H__
#define __IDT_H__

#include "../../include/types.h"

void IDT_setup(void);
void IDT_add_handler(unsigned char index, unsigned int handler);
void idt_remove_handler(uint8_t index);
bool_t idt_handler_in_use(uint8_t index);

/* extern assembly functions */
extern void ASM_IDT_SUBMIT(unsigned int *IDT);

/* ASSEMBLY ISR HANDLERS */
extern void ISR_STANDARD(void);
extern void ISR_00(void);
extern void ISR_01(void);
extern void ISR_02(void);
extern void ISR_03(void);
extern void ISR_04(void);
extern void ISR_05(void);
extern void ISR_06(void);
extern void ISR_07(void);
extern void ISR_08(void);
extern void ISR_09(void);
extern void ISR_0A(void);
extern void ISR_0B(void);
extern void ISR_0C(void);
extern void ISR_0D(void);
extern void ISR_0E(void);

extern void ISR_20(void);
extern void ISR_21(void);

extern void ISR_80(void);

#endif