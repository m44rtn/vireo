/*
MIT license
Copyright (c) 2022 Maarten Vermeulen

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


// includes from the syslib
#include "kernel.h"
#include "api.h"
#include "exit_code.h"
#include "driver.h"

// includes from ps2keyb
#include "ps2keyb.h"

#define KEYB_INT            0x21
#define KEYB_PORT           0x60

typedef struct ps2keyb_api_req
{
    syscall_hdr_t hdr;
    void *params;
} __attribute__((packed)) ps2keyb_api_req;

// prototypes
uint8_t inb(uint16_t _port);
void ps2keyb_isr(void);
void ps2keyb_api_handler(void *req);

uint8_t inb(uint16_t _port)
{
	uint8_t rv;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

void ps2keyb_isr(void)
{
    uint8_t c = inb(KEYB_PORT);

    // TODO: make buffers
}

void ps2keyb_api_handler(void *req)
{
    ps2keyb_api_req *r = req;

    switch((r->hdr.system_call) & 0xFF)
    {
        case PS2KEYB_CALL_REGISTER_OUTBFR:
            // TODO
        break;

        case PS2KEYB_CALL_DEREGISTER_OUTBFR:
            // TODO
        break;

        case PS2KEYB_CALL_LAST_KEY:
        break;

        default:
            r->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
    }
}

err_t main(void)
{
    // TODO:
    // set-up keyboard

    // FIXME: The kernel does not use the STANDARD_ISR_HANDLER().
    //        This means it is impossible to execute an ISR outside the kernel
    //        at the moment. 
    // FIXME: The kernel should use iret to return out of 'interrupt mode' so to speak
    //        otherwise the system will not be able to receive new interrupts because
    //        some programs may do a lot of processing during an interrupt handler, which
    //        is not recommended within an ISR anyway.
    kernel_add_interrupt_handler(ps2keyb_isr, KEYB_INT);
    
    // FIXME: driver filename is not set by SYSCALL_REQUEST_API_SPACE.
    //        instead, the filename of the program calling the driver
    //        is used...
    // FIXME: Allow the driver listing to return the driver's filename.
    //        This makes it easier to find the API space of a specific type
    //        of driver (e.g., a keyboard driver). 
    api_space_t space = api_get_api_space(ps2keyb_api_handler);

    if(space == (api_space_t) MAX)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    


    return EXIT_CODE_GLOBAL_SUCCESS;
}
