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

#ifndef QUIET_KERNEL
    #include "../screen/screen_basic.h"
#endif

#define DRIVER_STRUCT_HEXSIGN   0xB14D05
#define DRIVER_STRUCT_CHARSIGN  "VIREODRV"


struct DRIVER_SEARCH
{
    unsigned int sign1;
    char sign2[8];
    unsigned int type;
} __attribute__((packed));

static void driver_search_pciAll(void);


void driver_init(void)
{
    driver_search_pciAll();
}

static void driver_search_pciAll(void)
{
    uint8_t i;
    uint32_t info, driver_type, *driver_loc;
    struct DRIVER_SEARCH drv = {DRIVER_STRUCT_HEXSIGN, "VIREODRV", 0};
    uint32_t *devicelist = pciGetAllDevices();
    
    trace("&drv: 0x%x\n", &drv);
    trace("&start: 0x%x\n", memory_getKernelStart());
    trace("&end: 0x%x\n", memory_getMallocStart());
    trace("matchsize = %i\n", sizeof(struct DRIVER_SEARCH));

    /* search for internal drivers */
    for(i = 0; i < 255; ++i)
    {
        if(devicelist[i] == 0)
            break;

        info = pciGetInfo(devicelist[i]);
        driver_type = info | DRIVER_TYPE_PCI;
        trace("driver_type: %x\n", driver_type);

        drv.type = driver_type;

        driver_loc = memsrch((void *) &drv, sizeof(struct DRIVER_SEARCH), memory_getKernelStart(), memory_getMallocStart());

        trace("[DRIVER] found driver for device %x ", devicelist[i]);
        trace("at 0x%x\n", driver_loc);

    }

    demalloc(devicelist);
}

