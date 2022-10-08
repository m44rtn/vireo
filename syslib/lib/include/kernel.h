/*
MIT license
Copyright (c) 2021 Maarten Vermeulen

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

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "types.h"
#include "call.h"

#define KERNEL_REV_MAX_SIZE     8

#define KERNEL_INTERRUPT        0x21

typedef struct kernel_ver_t
{
    uint16_t major;
    uint16_t minor;
    uint32_t build;
    char rev[KERNEL_REV_MAX_SIZE];
} __attribute__((packed)) kernel_ver_t;

// returns full version string
char *kernel_get_version_str(void);

// returns kernel_ver_t with full information on the version number
kernel_ver_t *kernel_get_version_number(void);

// returns a list of all handlers registered to a specific interrupt number
void **kernel_get_free_interrupt_handlers(uint8_t _int);

// adds the interrupt handler at _handler to as the interrupt handler for int _int
err_t kernel_add_interrupt_handler(uint32_t _handler, uint8_t _int);

// removes the interrupt handler _handler registered to interrupt number _int
err_t kernel_remove_interrupt_handler(uint32_t _handler, uint8_t _int);

// returns the number of systicks passed
uint32_t kernel_get_systicks(void);

// delays until _ms milliseconds have passed
void kernel_sleep(uint32_t _ms);

#endif // __KERNEL_H__
