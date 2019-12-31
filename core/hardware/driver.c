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

/* This 'module' will handle drivers and such */
#include "driver.h"

#include "pci.h"

#include "../include/types.h"

#include "../memory/memory.h"

#define DRIVER_STRUCT_HEXSIGN   0xB14D05
#define DRIVER_STRUCT_CHARSIGN  "VIREODRV"

#define DRIVER_TYPE_PCI         0x01 << 24;

struct DRIVER_SEARCH
{
    unsigned int sign1;
    char sign2[8];
    unsigned int type;
} __attribute__((packed));

void driver_init(void)
{
    driver_search_pciAll();
}

static void driver_search_pciAll(void)
{
    uint8_t i;
    uint32_t info, driver_type;
    uint32_t *devicelist = pciGetAllDevices();
    
    /* search for internal drivers */
    for(i = 0; i < 256; ++i)
    {
        info = pciGetInfo(devicelist[i]);
        driver_type = info | DRIVER_TYPE_PCI;
        struct DRIVER_SEARCH drv = {DRIVER_STRUCT_HEXSIGN, DRIVER_STRUCT_CHARSIGN, driver_type};
        memsrch((void *) drv, sizeof(struct DRIVER_SEARCH), /* get start kernel from memory.c */, /*end is malloc start */);
    }
}