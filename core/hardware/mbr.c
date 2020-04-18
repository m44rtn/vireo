/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

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

#include "mbr.h"

#include "driver.h"
#include "pci.h"

#include "../include/types.h"

#include "../drv/IDE_commands.h"

/* remove */
#include "../screen/screen_basic.h"

typedef struct /* only the mbr info that's interesting to us */
{
    uint32_t start_LBA;
    uint32_t n_sectors;
    uint8_t type;
    uint8_t active;
} __attribute__((packed)) MBR_ENTRY_INFO;

typedef struct
{
    uint8_t disk;
    MBR_ENTRY_INFO mbr_entry_t[4];
} __attribute__((packed)) MBR;

/* enumerates the MBRs of all present (only IDE for now)  disks in the system */
void enumerateMBRs()
{
    uint32_t *drv;
    uint8_t *drives;
    
    drv = malloc(512);
    drives = (uint8_t *)(((uint32_t)drv) + sizeof(uint32_t)*5);
    
    /* TODO:
- use proper way to call the driver
 - get useful drives put them in MBR.disk
- enumerate MBR and put info in MBR.mbr_entry_t[]
    - clean-up
*/
    
    drv[0] = IDE_COMMAND_REPORTDRIVES;
    drv[1] = (uint32_t) (drives);
    driver_exec(pciGetInfo(0x010101) | DRIVER_TYPE_PCI, drv);
    
    drv[0] = IDE_COMMAND_READ;
    drv[1] = 0;
    drv[2] = 0;
    drv[3] = 1;
    drv[4] = 0x200400;
    driver_exec(pciGetInfo(0x010101) | DRIVER_TYPE_PCI, drv);
}