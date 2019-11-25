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

#include "pci.h"

#include "../include/types.h"

#include "../io/io.h"

#include "../screen/screen_basic.h"

/* I know there is a potential for more than 256 devices, but who cares
just to be nice though, I'll mark it as TODO */
uint32_t PCI_DEV_LIST[256];

static uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg);

void pci_init(void)
{	
    uint8_t i = 0;
    uint16_t bus, device, func, class;
    uint16_t vendorid, deviceid;
    uint32_t val;

    for(bus = 0; bus < 256; ++bus)
    {
        for(device = 0; device < 32; ++device)
        {
            for(func = 0; func < 8; ++func)
            {
                vendorid = (uint16_t) (pciConfigRead((uint8_t) bus, (uint8_t) device, (uint8_t) func, 0x00) & 0xFFFF);
                deviceid = (uint16_t) (pciConfigRead((uint8_t) bus, (uint8_t) device, (uint8_t) func, 0x00) >> 16) & 0xFFFF;

                if(vendorid == 0xFFFF)
                    continue;
                
                val = pciConfigRead((uint8_t) bus, (uint8_t) device, (uint8_t) func, 2);
                val = val >> 16;

                class = (uint16_t) (val >> 8);

                #ifndef QUIET_KERNEL
                screen_set_hexdigits(4);
                trace((char *) "[PCI] found device %x:", deviceid);
                trace((char *) "%x\n", vendorid);
                #endif
                
                PCI_DEV_LIST[i] = (uint32_t) ((bus & 0xFF << 24) | (device << 16) | (func << 8) | (class & 0xFF));
            }
        }
    }

    #ifndef QUIET_KERNEL
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);
    print((char *) "[PCI] OK\n\n");
    #endif
}

static uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg){
	
	/*just so it looks nice*/
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint32_t lreg = (uint32_t) reg;
	uint32_t tmmp;
	
	/* create config adress */
	uint32_t address = (uint32_t) ((lbus << 16) | (ldevice << 11) | (lfunc << 8)  | (lreg << 2) | (uint32_t) 0x80000000);
	
	ASM_OUTL(0x0cf8, address);
	tmmp = ASM_INL(0x0cfc);
	
	return tmmp;
}

uint8_t pciGetInterruptLine(uint8_t bus, uint8_t device, uint8_t func)
{
	uint32_t thing = pciConfigRead(bus, device, func, 0x0F);
	uint8_t interruptline = (uint8_t) thing;
	return interruptline;
}