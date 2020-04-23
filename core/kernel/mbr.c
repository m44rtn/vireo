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

#include "../memory/memory.h"

#include "../util/util.h"

#include "../drv/IDE_commands.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#define NUM_DRIVES    IDE_DRIVER_MAX_DRIVES /*TODO: + floppy's + ... */

#define MBR_PARTENTRY_START 0x1BE
#define MBR_PARTENTRY_SIZE  16

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

MBR DISKS[NUM_DRIVES];
uint8_t nDisks;

#ifndef NO_DEBUG_INFO
static void MBR_printAll(void);
#endif

static uint8_t MBR_getIDEDrives(uint8_t *drives);
static void IDE_readMBR(uint8_t disk, uint32_t *drv, uint32_t controller, uint8_t *buf);

/* enumerates the MBRs of all present (only IDE for now)  disks in the system */
void MBR_enumerate(void)
{
    uint32_t *drv, *devicelist, *mbr_entry, IDE_ctrl;
    uint8_t *drives, *mbr, disks;
    uint8_t i, j;

    drv = malloc(DRIVER_COMMAND_PACKET_LEN*sizeof(uint32_t) + IDE_DRIVER_MAX_DRIVES*sizeof(uint32_t));
    drives = (uint8_t *)(((uint32_t)drv) + sizeof(uint32_t)*DRIVER_COMMAND_PACKET_LEN);

    devicelist = pciGetDevices(0x01, 0x01);
    IDE_ctrl = devicelist[1];
    free(devicelist);

    drv[0] = IDE_COMMAND_REPORTDRIVES;
    drv[1] = (uint32_t) (drives);
    driver_exec(pciGetInfo(IDE_ctrl) | DRIVER_TYPE_PCI, drv);

    nDisks = disks = MBR_getIDEDrives(drives);

    if(nDisks < 1)
      return;

    mbr = (uint8_t *) malloc(512);

    /* this is IDE only, when floppy's are introduced this should be moved
to a seperate function */
    for(i = 0; i < disks; ++i)
    {
        IDE_readMBR(DISKS[i].disk, drv, IDE_ctrl, mbr);

        for(j = 0; j < 4; ++j)
        {
            mbr_entry = (uint32_t *) &mbr[MBR_PARTENTRY_START + j*MBR_PARTENTRY_SIZE];

            DISKS[i].mbr_entry_t[j].active = (uint8_t) (mbr_entry[0] & 0xFFU);
            DISKS[i].mbr_entry_t[j].type = (uint8_t) (mbr_entry[1] & 0xFFU);
            DISKS[i].mbr_entry_t[j].start_LBA = mbr_entry[2];
            DISKS[i].mbr_entry_t[j].n_sectors = mbr_entry[3];
        }
    }

    free(mbr);
    free(drv);

    #ifndef NO_DEBUG_INFO
    MBR_printAll();
    #endif
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

      trace((char *)"[PARTITIONS] HD%i ", DISKS[i].disk);
      trace((char *)"p%i: ", j);
      trace((char *)"lba %i, ", DISKS[i].mbr_entry_t[j].start_LBA);
      trace((char *)"active: %x, ", DISKS[i].mbr_entry_t[j].active);
      trace((char *)"sectors: %i, ", DISKS[i].mbr_entry_t[j].n_sectors);
      trace((char *)"type: %x\n", DISKS[i].mbr_entry_t[j].type);
    }
  }

  print((char *)"\n");
}
#endif


static uint8_t MBR_getIDEDrives(uint8_t *drives)
{
    uint8_t i = 0, disks = 0;
    for(; i < IDE_DRIVER_MAX_DRIVES; ++i)
        if(drives[i] == IDE_DRIVER_TYPE_PATA) DISKS[disks++].disk = i;

    return disks;
}

static void IDE_readMBR(uint8_t disk, uint32_t *drv, uint32_t controller, uint8_t *buf)
{
    memset((char *) drv,DRIVER_COMMAND_PACKET_LEN*sizeof(uint32_t), 0);
    drv[0] = IDE_COMMAND_READ;
    drv[1] = (uint32_t) disk;
    drv[2] = 0U; /* sector 0 for mbr */
    drv[3] = 1U; /* one sector read */
    drv[4] = (uint32_t) buf;
    driver_exec(pciGetInfo(controller) | DRIVER_TYPE_PCI, drv);
}
