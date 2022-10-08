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

#ifndef __ISR_H__
#define __ISR_H__

#include "../../include/types.h"

void isr_set_extern_handler(unsigned char type, void *handler);
void **isr_get_extern_handlers(void);

void ISR_STANDARD_HANDLER(void);

void ISR_00_HANDLER(void);
void ISR_01_HANDLER(void);
void ISR_05_HANDLER(void);
void ISR_06_HANDLER(void);
void ISR_0D_HANDLER(void);
void ISR_0E_handler(unsigned int error_code);

void ISR_20_HANDLER(void);

void ISR_80_HANDLER(void *eip, void *req);

#endif