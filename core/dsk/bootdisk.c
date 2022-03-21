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

#include "bootdisk.h"
#include "diskio.h"
#include "diskdefines.h"

#include "../boot/loader.h"
#include "../memory/memory.h"
#include "../util/util.h"

#define FLOPPY          0x00 // until 0x7F
#define HARDDISK        0x80 // until 0xFF
#define CD_DRIVE        0xE0 // also counts as 97th harddisk

#define DRIVE_ID_BUFFER_SIZE   32

char *bootdisk(void)
{
    uint32_t bootdisk = loader_get_boot_drive();
    const uint8_t type = (const uint8_t) ((bootdisk >> 24) & 0xFF);
    const uint8_t part = (const uint8_t) ((bootdisk >> 16) & 0xFF);

    char *id = NULL;

    if(type < HARDDISK) // Floppy
        return NULL; // not implemented
    else if(type >= HARDDISK && type != CD_DRIVE) // hard disk
        id = bootdisk_harddisk_drive_number(type, part);
    else if(type == CD_DRIVE)
        id = bootdisk_cddrive_drive_number(); 

    return id;
}

char *bootdisk_harddisk_drive_number(const uint8_t type, const uint8_t part)
{
    uint8_t *disks = diskio_reportDrives();
    uint8_t disknumber = (uint8_t) ((type) -  HARDDISK);

    uint8_t i = 0, j = 0;
    for(; i < MAX_DRIVES; ++i)
    {
        if(disks[i] == DRIVE_TYPE_IDE_PATA)
            ++j;
        if(j == disknumber + 1 /* offset since drive number starts at 0 */)
            break;
    }

    char *id = kmalloc(DRIVE_ID_BUFFER_SIZE);
    drive_convert_to_drive_id(i, id);

    // add the partition number to the drive id
    id[strlen(id)] = 'P';
    char *p = intstr(part);
    memcpy(&id[strlen(id)], p, strlen(p));

    return id;
}

char *bootdisk_cddrive_drive_number(void)
{
    // it seems to be the case that the BIOS only tries the first CD drive and
    // if this one isn't bootable it tries the harddisk (even though the CD drives are 
    // first in the boot order); at least this is the case for VirtualBox 6.1
    // therefore this function does not check if the first CD is actually the one containing vireo.sys

    uint8_t *disks = diskio_reportDrives();
    uint8_t i = 0;

    for(; i < MAX_DRIVES; ++i)
        if(disks[i] == DRIVE_TYPE_IDE_PATAPI)
            break;

    char *id = kmalloc(DRIVE_ID_BUFFER_SIZE);
    drive_convert_to_drive_id(i, id);

    return id;
}
