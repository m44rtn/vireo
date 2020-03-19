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

#include "pic.h"

#include "../include/types.h"

#include "../io/io.h"

#include "timer.h"

#define PIC_MASTER_CMNDSTAT     0x20
#define PIC_MASTER_IMRDATA      0x21

#define PIC_SLAVE_CMNDSTAT      0xA0
#define PIC_SLAVE_IMRDATA       0xA1

#define PIC_READ_ISR            0x0B

/* does not return an exit code (for now) */
void PIC_controller_setup(void)
{
    /* basically, this only remaps the PICs.
    order: ICW1, ICW2, ICW3 and ICW4.
    IOWAIT() is there for older hardware
    */

    outb(PIC_MASTER_CMNDSTAT, 0x11);
    ASM_IOWAIT();
    outb(PIC_SLAVE_CMNDSTAT,  0x11);
    
    ASM_IOWAIT();

    outb(PIC_MASTER_IMRDATA, 0x20);
    ASM_IOWAIT();
    outb(PIC_SLAVE_IMRDATA,  0x28);

    ASM_IOWAIT();

    outb(PIC_MASTER_IMRDATA, 0x04);
    ASM_IOWAIT();
    outb(PIC_SLAVE_IMRDATA,  0x02);

    ASM_IOWAIT();

    outb(PIC_MASTER_IMRDATA, 0x01);
    ASM_IOWAIT();
    outb(PIC_SLAVE_IMRDATA,  0x01);

    ASM_IOWAIT();
    outb(PIC_MASTER_IMRDATA, 0x00);
    ASM_IOWAIT();
    outb(PIC_SLAVE_IMRDATA,  0x00);

    /* unmask all */
    ASM_IOWAIT();
    outb(PIC_MASTER_CMNDSTAT, 0x00);
    ASM_IOWAIT();
    outb(PIC_SLAVE_CMNDSTAT,  0x00);

    /* then we configure the PIT */
    PITInit();
}

void PIC_mask(unsigned char IRQ)
{
    uint8_t mask;
    uint16_t port = PIC_MASTER_IMRDATA;
    
    if(IRQ > 7)
    {
        IRQ = (uint8_t) (IRQ - 8);
        port = PIC_SLAVE_IMRDATA;
    }

    mask = (uint8_t) ((inb(port)) | (uint32_t)(1 << IRQ));
    outb(port, mask);
}

void PIC_umask(unsigned char IRQ)
{
    uint8_t mask;
    uint16_t port = PIC_MASTER_IMRDATA;
    
    if(IRQ > 7)
    {
        IRQ = (uint8_t)  (IRQ - 8);
        port = PIC_SLAVE_IMRDATA;
    }

    mask = (uint8_t) (inb(port) & (uint32_t)~(1 << IRQ));
    outb(port, mask);
}

void PIC_EOI(unsigned char IRQ)
{
    uint16_t port = PIC_MASTER_CMNDSTAT;
    outb(port, 0x20);

    if(IRQ > 7)
    {
        port = PIC_SLAVE_CMNDSTAT;
        outb(port, 0x20);
    }
}

uint16_t PIC_read_ISR(void)
{
    outb(PIC_MASTER_CMNDSTAT, PIC_READ_ISR);
    outb(PIC_SLAVE_CMNDSTAT,  PIC_READ_ISR);

    return (uint16_t)((inb(PIC_SLAVE_CMNDSTAT) << 8U) | inb(PIC_MASTER_CMNDSTAT));    
}