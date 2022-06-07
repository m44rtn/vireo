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

#include "mbr.h"
#include "diskio.h"

#include "../hardware/driver.h"

#include "../drv/COMMANDS.H"

#include "../include/types.h"
#include "../dsk/diskdefines.h"

#include "../memory/memory.h"

#include "../util/util.h"

#include "../drv/IDE_commands.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#define MBR_PARTENTRY_START 0x1BE
#define MBR_PARTENTRY_SIZE  16

#define MBR_MAX_PARTITIONS  4 // per disk

typedef struct /* only the mbr info that's interesting to us */
{
  uint8_t active;
  uint8_t type;
  uint32_t start_LBA;
  uint32_t n_sectors;
} __attribute__((packed)) MBR_ENTRY_INFO;

typedef struct
{
  uint8_t disk;
  MBR_ENTRY_INFO mbr_entry_t[4];
} __attribute__((packed)) MBR;

MBR DISKS[MAX_DRIVES];
uint8_t nDisks;

#ifndef NO_DEBUG_INFO
static void MBR_printAll(void);
#endif

static uint8_t MBR_getIDEDrives(uint8_t *drives);

/* enumerates the MBRs of all present (only IDE for now) harddisks in the system */
void MBR_enumerate(void)
{
    uint32_t *mbr_entry;
    uint8_t *mbr, disks;
    uint8_t i, j, error;

    memset(&DISKS[0], sizeof(MBR) * MAX_DRIVES, 0xFF);

    uint8_t *drives = diskio_reportDrives();
    nDisks = disks = MBR_getIDEDrives(drives);

    if(nDisks < 1)
        return;

    mbr = (uint8_t *) kmalloc(DEFAULT_SECTOR_SIZE);

    /* this is IDE only, if floppy's are introduced this should be moved
        to a seperate function */
    for(i = 0; i < disks; ++i)
    {
        if(drives[i] != DRIVE_TYPE_IDE_PATA)
            continue;
        
        error = read(DISKS[i].disk, 0U, 1U, mbr);
        
        if(error)
            break;

        // read all partition entries
        for(j = 0; j < MBR_MAX_PARTITIONS; ++j)
        {
            mbr_entry = (uint32_t *) &mbr[MBR_PARTENTRY_START + j*MBR_PARTENTRY_SIZE];

            DISKS[i].mbr_entry_t[j].active = (uint8_t) (mbr_entry[0] & 0xFFU);
            DISKS[i].mbr_entry_t[j].type = (uint8_t) (mbr_entry[1] & 0xFFU);
            DISKS[i].mbr_entry_t[j].start_LBA = mbr_entry[2];
            DISKS[i].mbr_entry_t[j].n_sectors = mbr_entry[3];
        }
    }

    kfree(mbr);
    kfree(drives);

    mbr_initialize_fs_drivers();

    #ifndef NO_DEBUG_INFO
    if(!error)
        MBR_printAll();
    #endif
}

void mbr_initialize_fs_drivers(void)
{
    uint32_t drv[5];

    for(uint32_t disk = 0; disk < MAX_DRIVES; ++disk)
        for(uint8_t part = 0; part < MBR_MAX_PARTITIONS; ++part)
        {
            uint8_t type = DISKS[disk].mbr_entry_t[part].type;

            if(!type || type == 0xFF)
                continue;

            driver_addInternalDriver((type | DRIVER_TYPE_FS));
            
            drv[0] = DRV_COMMAND_INIT;
            drv[1] = disk;
            drv[2] = part;
            drv[3] = type;
            driver_exec_int((type | DRIVER_TYPE_FS), &drv[0]);
        }
}

uint32_t MBR_getStartLBA(uint8_t disk, uint8_t partition)
{
    return DISKS[disk].mbr_entry_t[partition].start_LBA;
}

uint32_t mbr_get_sector_count(uint8_t disk, uint8_t partition)
{
    return DISKS[disk].mbr_entry_t[partition].n_sectors;
}

uint8_t mbr_get_type(uint8_t disk, uint8_t partition)
{
    return DISKS[disk].mbr_entry_t[partition].type;
}

#ifndef NO_DEBUG_INFO
static void MBR_printAll(void)
{
    uint8_t i, j;
    for(i = 0; i < nDisks; ++i)
    {
    for(j = 0; j < 4; ++j)
    {
        if(!DISKS[i].mbr_entry_t[j].start_LBA)
        continue;

        print_value("[PARTITIONS] HD%i", DISKS[i].disk);
        print_value("p%i: ", j);
        print_value("lba %i, ", DISKS[i].mbr_entry_t[j].start_LBA);
        print_value("active: %x, ", DISKS[i].mbr_entry_t[j].active);
        print_value("sectors: %i, ", DISKS[i].mbr_entry_t[j].n_sectors);
        print_value("type: %x\n", DISKS[i].mbr_entry_t[j].type);
    }
    }

    print("\n");
}
#endif


static uint8_t MBR_getIDEDrives(uint8_t *drives)
{
    uint8_t i = 0, disks = 0;
    for(; i < IDE_DRIVER_MAX_DRIVES; ++i)
    {
        if(drives[i] == DRIVE_TYPE_IDE_PATA) DISKS[disks++].disk = i;
    }

    return disks;
}
