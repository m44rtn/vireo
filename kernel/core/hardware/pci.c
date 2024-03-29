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

#include "pci.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../io/io.h"

#include "../memory/memory.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#define PCI_DEVLIST_LENGTH          100 // devices

#define PCI_CONFIG_ADDR             0xcf8
#define PCI_CONFIG_DATA             0xcfc

#define PCI_CLASS_CODE              24 // starting bit
#define PCI_SUBCLASS                16 // starting bit

typedef struct
{
    uint32_t device;
    uint32_t Reg0;
    uint8_t subclass;
} PCI_DEV;


/* While PCI does support more devices, it is unrealistic that
   Vireo would ever be in a realization where it should support 
   more than 100 devices.
 This list is 900 bytes in size*/
static PCI_DEV PCI_DEV_LIST[PCI_DEVLIST_LENGTH];

static uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg);

void pci_init(void)
{	
    uint32_t i = 0;
    uint16_t bus, device, func;
    uint8_t class;
    uint32_t val;

    for(bus = 0; bus < 256; ++bus)
    {
        for(device = 0; device < 32; ++device)
        {
            for(func = 0; func < 8; ++func)
            {
                uint16_t vendorid = (uint16_t) (pciConfigRead((uint8_t) bus, (uint8_t) device, (uint8_t) func, 0x00) & 0xFFFF);
                uint16_t deviceid = (uint16_t) (pciConfigRead((uint8_t) bus, (uint8_t) device, (uint8_t) func, 0x00) >> 16) & 0xFFFF;

                if(vendorid == 0xFFFF)
                    continue;
                
                val = pciConfigRead((uint8_t) bus, (uint8_t) device, (uint8_t) func, 2);
                uint8_t subclass = (val >> PCI_SUBCLASS) & 0xFF;
                class = (uint8_t) (val >> PCI_CLASS_CODE);

                #ifndef NO_DEBUG_INFO
                screen_set_hexdigits(4);
                print_value( "[PCI] Found device %x:", deviceid);
                print_value( "%x\n", vendorid);
                #endif
                
                PCI_DEV_LIST[i].device   = (uint32_t) ((bus & 0xFF << 24) | (device << 16) | (func << 8) | (class));
                PCI_DEV_LIST[i].Reg0     = (uint32_t) (deviceid << 16) | vendorid;
                PCI_DEV_LIST[i].subclass = subclass;
                ++i;
            }
        }
    }

    #ifndef NO_DEBUG_INFO
    screen_set_hexdigits(SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT);
    print( "[PCI] OK\n\n");
    #endif
}

uint32_t *pciGetDevices(uint8_t class, uint8_t subclass)
{
    uint8_t list_class, list_subclass;
    uint32_t *devicelist = kmalloc((PCI_DEVLIST_LENGTH + 1) * sizeof(uint32_t));

    uint32_t i = 0, j = 1;
    for(; i < PCI_DEVLIST_LENGTH; ++i)
    {
        list_class = (uint8_t) (PCI_DEV_LIST[i].device & 0xFF);
        list_subclass = PCI_DEV_LIST[i].subclass;
        if(list_class == class && list_subclass == subclass)
        {
            devicelist[j] = (uint32_t) (PCI_DEV_LIST[i].device);
            ++j;
        }
    }

    /* store useful information in the first entry like the length of the array (maybe more in the future) */
    devicelist[0] = (uint32_t) j;
    return devicelist;
}

uint32_t *pciGetAllDevices(void)
{
    uint32_t i;
    uint32_t *devicelist = kmalloc(PCI_DEVLIST_LENGTH * sizeof(uint32_t));
    
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
    uint32_t index = pci_get_device_index(device);
    
    /* class and subclass (respectively) */
    uint32_t info = ((device & 0xFF) << 8) | PCI_DEV_LIST[index].subclass;

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

uint32_t pci_get_device_index(uint32_t device)
{
    uint32_t i = 0;
    for(; i < PCI_DEVLIST_LENGTH; ++i)
    {
        if(PCI_DEV_LIST[i].device == device)
            break;
    }

    return i;
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

uint32_t pci_read_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t addr = (uint32_t) (0x80000000u | (uint8_t)(bus << 16) | (uint8_t)(dev << 11) | (uint8_t)(func << 8) | (uint8_t)(offset & 0xfc));
    outl(PCI_CONFIG_ADDR, addr);
    return (uint32_t) inl(PCI_CONFIG_DATA);
}

/* read used by pci.c itself, it's left here because it is slightly different compared to the above
    function and may work differently */
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
