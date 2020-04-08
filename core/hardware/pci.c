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
#include "../include/exit_code.h"

#include "../io/io.h"

#include "../memory/memory.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#define PCI_DEVLIST_LENGTH          64

typedef struct
{
    uint32_t device;
    uint32_t Reg0;
} PCI_DEV;


/* PCI does support more than 64 devices, but Vireo doesn't (unless you change the define of course)
    - 512 bytes */
static PCI_DEV PCI_DEV_LIST[PCI_DEVLIST_LENGTH];

static uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg);

void pci_init(void)
{	
    uint32_t i = 0;
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

                #ifndef NO_DEBUG_INFO
                screen_set_hexdigits(4);
                trace((char *) "[PCI] Found device %x:", deviceid);
                trace((char *) "%x\n", vendorid);
                #endif
                
                PCI_DEV_LIST[i].device = (uint32_t) ((bus & 0xFF << 24) | (device << 16) | (func << 8) | (class & 0xFF));
                PCI_DEV_LIST[i].Reg0   = (uint32_t) (deviceid << 16) | vendorid;
                ++i;
            }
        }
    }

    #ifndef NO_DEBUG_INFO
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);
    print((char *) "[PCI] OK\n\n");
    #endif
}

uint32_t *pciGetDevices(uint8_t class, uint8_t subclass)
{
    uint8_t class_fromlist, bus, device, func, answer;
    uint32_t *devicelist = malloc(512 * sizeof(uint32_t));

    uint32_t i = 0, j = 1;
    for(; i < PCI_DEVLIST_LENGTH - 1; ++i)
    {
        class_fromlist = (uint8_t) (PCI_DEV_LIST[i].device & 0xFF);
        if(class_fromlist == class)
        {
            bus    = (uint8_t) ((PCI_DEV_LIST[i].device >> 24) & 0xFF);
            device = (uint8_t) ((PCI_DEV_LIST[i].device >> 16) & 0xFF);
            func   = (uint8_t) ((PCI_DEV_LIST[i].device >> 8)  & 0xFF);   

            answer = ((pciConfigRead(bus, device, func, 0x02) >> 16) & 0xFF);

            if(answer == subclass)
            {
                devicelist[j] = (uint32_t) (PCI_DEV_LIST[i].device);
                ++j;
            }
        }
    }

    /* store useful information in the first entry like the length of the array (maybe more in the future) */
    devicelist[0] = (uint32_t) j;
    return devicelist;
}

uint32_t *pciGetAllDevices(void)
{
    uint32_t i;
    uint32_t *devicelist = malloc(256 * sizeof(uint32_t));
    
    for(i = 0; i < PCI_DEVLIST_LENGTH; ++i)
        devicelist[i] = PCI_DEV_LIST[i].device;
  
        
    return devicelist;
}

uint8_t pciGetInterruptLine(uint8_t bus, uint8_t device, uint8_t func)
{
	uint32_t thing = pciConfigRead(bus, device, func, 0x0F);
	uint8_t interruptline = (uint8_t) thing;
	return interruptline;
}

uint32_t pciGetInfo(uint32_t device)
{
    uint32_t info, answer;

    uint8_t bus     = (uint8_t) ((device >> 24) & 0xFF);
    uint8_t dev     = (uint8_t) ((device >> 16) & 0xFF);
    uint8_t func    = (uint8_t) ((device >> 8)  & 0xFF);

    /* get the subclass */
    answer = ((pciConfigRead(bus, dev, func, 0x02) >> 16) & 0xFF);
    
    /* class and subclass */
    info = ((device & 0xFF) << 8) | (answer & 0xFF);

    return info;
}

/* gets register 0 (deviceID and vendorID) */
uint32_t pciGetReg0(uint32_t device)
{
    uint32_t i;

    for(i = 0; i < PCI_DEVLIST_LENGTH; ++i)
    {
        if(PCI_DEV_LIST[i].device == device)
            break;
    }

    return PCI_DEV_LIST[i].Reg0;
}

uint32_t pciGetDeviceByReg0(uint32_t Reg0)
{
    uint32_t i;

    for(i = 0; i < PCI_DEVLIST_LENGTH; ++i)
    {
        if(PCI_DEV_LIST[i].Reg0 == Reg0)
            break;
    }

    return PCI_DEV_LIST[i].device;
}

uint32_t pciGetBar(uint32_t device, uint8_t bar)
{
    uint8_t bus     = (uint8_t) ((device >> 24) & 0xFF);
    uint8_t dev     = (uint8_t) ((device >> 16) & 0xFF);
    uint8_t func    = (uint8_t) ((device >> 8)  & 0xFF);

    if(bar < PCI_BAR0 || bar > PCI_BAR5)
        return 0;
    
    return pciConfigRead(bus, dev, func, bar);
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
	
	outl(0x0cf8, address);
	tmmp = (uint32_t) inl(0x0cfc);
	
	return tmmp;
}
