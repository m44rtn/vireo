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
#include "../kernel/exec.h"

#include "../memory/memory.h"

#ifndef NO_DEBUG_INFO
    #include "../screen/screen_basic.h"
#endif

#define DRIVER_STRUCT_HEXSIGN   0xB14D05
#define DRIVER_STRUCT_CHARSIGN  "VIREODRV"

#define DRIVER_MAX_SUPPORTED    64

struct DRIVER_SEARCH
{
    unsigned int sign1;
    char sign2[8];
    unsigned int type;
} __attribute__((packed));

typedef struct
{
    uint32_t device;
    uint32_t type;
    uint32_t *driver;
} DRIVER_LIST;

/* with 64 drivers this list is 768 bytes */
DRIVER_LIST drv_list[DRIVER_MAX_SUPPORTED];
uint8_t cur_devices = 0;

static void driver_search_pciAll(void);

/* TODO:
        - add functionality to add and remove new/old drivers */

/* A driver command packet is an array of five uint32_t's. 
    The packet is as follows:

    0: command
    1-4: parameter1 - parameter4
    */


void driver_init(void)
{
    driver_search_pciAll();

#ifndef NO_DEBUG_INFO
    print((char *) "\n");
#endif
}

void driver_exec(uint32_t type, uint32_t *data)
{
    uint8_t i;

    /* the kernel only supports one type for a driver, so this means we can use it as a key (which is easier to test with since you can just look it up on google). */
    for(i = 0; i < DRIVER_MAX_SUPPORTED; ++i)
        if(drv_list[i].type == type)
            break;
    
    if(i >= DRIVER_MAX_SUPPORTED)
        return;

    EXEC_CALL_FUNC(drv_list[i].driver, (uint32_t *) data);
}

/* FYI: not all only internal drivers */
static void driver_search_pciAll(void)
{
    uint8_t i;
    uint32_t info, driver_type, *driver_loc;
    struct DRIVER_SEARCH drv = {DRIVER_STRUCT_HEXSIGN, "VIREODRV", 0};
    uint32_t *devicelist = pciGetAllDevices();

    /* search for internal drivers */
    for(i = 0; i < 255; ++i)
    {
        if(devicelist[i] == 0)
            break;

        info = pciGetInfo(devicelist[i]);
        driver_type = info | DRIVER_TYPE_PCI;

        drv.type = driver_type;
        driver_loc = memsrch((void *) &drv, sizeof(struct DRIVER_SEARCH), memory_getKernelStart(), memory_getMallocStart());

        if(driver_loc)
        {
            drv_list[cur_devices].device = devicelist[i];
            drv_list[cur_devices].type = driver_type;
            /* store the address of the driver's handler */
            drv_list[cur_devices].driver = (uint32_t *) *( (uint32_t*) ((uint32_t)driver_loc + sizeof(struct DRIVER_SEARCH))); /* I'm sorry */
            
            #ifndef NO_DEBUG_INFO
            trace((char *) "[DRIVER SUBSYSTEM] Found driver for device %x @ ", pciGetReg0(drv_list[cur_devices].device));
            trace((char *) "0x%x\n", (uint32_t) drv_list[cur_devices].driver);
            #endif

            ++cur_devices;    
        }

    }
    free(devicelist);
}

