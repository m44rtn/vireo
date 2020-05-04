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

#include "diskio.h"

#include "../include/types.h"
#include "../include/drivetypes.h"

#include "../hardware/driver.h"
#include "../hardware/pci.h"

#include "../memory/memory.h"

#include "../drv/IDE_commands.h"

#define DISKIO_MAX_DRIVES 4 /* max. 4 IDE drives, (TODO:) max. 2 floppies */

typedef struct{
    uint8_t diskID;
    uint8_t disktype;
    uint16_t controller_info;
}__attribute__((packed)) DISKINFO;

DISKINFO disk_info_t[DISKIO_MAX_DRIVES];

void diskio_init(void)
{
    uint8_t i;
    uint32_t *devicelist, IDE_ctrl;
    uint32_t *drv = malloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint8_t *drives = (uint8_t *)((uint32_t)drv) + sizeof(uint32_t)*DRIVER_COMMAND_PACKET_LEN;

    devicelist = pciGetDevices(0x01, 0x01);
    IDE_ctrl = devicelist[1];
    free(devicelist);

    drv[0] = IDE_COMMAND_REPORTDRIVES;
    drv[1] = (uint32_t) (drives);
    driver_exec(pciGetInfo(IDE_ctrl) | DRIVER_TYPE_PCI, drv);

    for(i = 0; i < DISKIO_MAX_DRIVES; ++i)
    {
        disk_info_t[i].disktype = (uint8_t) drives[i];
        disk_info_t[i].diskID = i; 
        disk_info_t[i].controller_info = (uint16_t) pciGetInfo(IDE_ctrl);
    }

    free(drv);
}

uint8_t *diskio_reportDrives(void)
{
    uint32_t i = 0;
    uint8_t *drive_list = (uint8_t *) malloc(DISKIO_MAX_DRIVES*sizeof(uint32_t));

    for(; i < IDE_DRIVER_MAX_DRIVES; ++i)
        drive_list[i] = disk_info_t[i].disktype;

    return drive_list;
}

void READ(unsigned char drive, unsigned int LBA, unsigned int sctrRead, unsigned char *buf)
{
    uint32_t *drv = malloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint32_t command = (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATA) ?
            IDE_COMMAND_READ : NULL; /* TODO: make NULL floppy command */

    drv[0] = command;
    drv[1] = (uint32_t) (drive);
    drv[2] = LBA;
    drv[3] = sctrRead;
    drv[4] = (uint32_t) (buf);

    driver_exec((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);

    free(drv);
}

void WRITE(unsigned char drive, unsigned int LBA, unsigned int sctrWrite, unsigned char *buf)
{
    uint32_t *drv = malloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint32_t command = (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATA) ?
            IDE_COMMAND_WRITE : NULL; /* TODO: make NULL floppy command */

    drv[0] = command;
    drv[1] = (uint32_t) (drive);
    drv[2] = LBA;
    drv[3] = sctrWrite;
    drv[4] = (uint32_t) (buf);

    driver_exec((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);

    free(drv);
}