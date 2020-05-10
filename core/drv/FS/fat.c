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

#include "fat.h"

#include "../../include/types.h"
#include "../../include/diskstuff.h"

#include "../../hardware/driver.h"

#include "../../kernel/mbr.h"

#include "../COMMANDS.H"
#include "../FS_TYPES.H"

#define FAT_FS_ID32 0x0B

typedef struct {
    char bootstuff[3];
    uint8_t OEM[8];
    uint16_t bSector;
    uint8_t SectClust;
    uint16_t resvSect;
    uint8_t nFAT;
    uint16_t nDirEnt;
    uint16_t nSect;
    uint8_t MDT; /*media descriptor type*/
    uint16_t sectFAT;
    uint16_t SectTrack;
    uint16_t HeadsTrack;
    uint32_t HiddenSect;
    uint32_t lnSect;
} __attribute__((packed)) BPB;

typedef struct{
    BPB bpb;
    uint32_t sectFAT32;
    uint16_t flags;
    uint16_t FATver;
    uint32_t clustLocRootdir;
    uint16_t FSinfo;
    uint16_t sectBootbackup;
    uint8_t resv[12];
    uint8_t driveNum;
    uint8_t NT;
    uint8_t Sign;
    uint32_t volID;
    uint8_t volName[11];
    uint8_t SysID[8];
    uint8_t bootcode[420];
    uint16_t bootSign;
} __attribute__((packed)) FAT32_EBPB;

typedef struct{
    char name[8];
    char ext[3]; 
    uint8_t attrib;
    uint8_t uAttrib; 

    char undelete;
    uint16_t cTime; 
    uint16_t cDate; 
    uint16_t aDate; 
    uint16_t clHi; 

    uint16_t mTime; 
    uint16_t mDate; 
    uint16_t clLo;
    uint32_t fSize; 
} __attribute__ ((packed)) FAT32_DIR;

typedef struct{
    uint8_t drive;
    uint8_t FStype;

    uint32_t lbaFAT;
} __attribute__((packed)) FS_INFO;

/* 108 bytes */
FS_INFO partition_info_t[DISKSTUFF_MAX_DRIVES*18]; /* maximum partitions: 4 per IDE drive + two floppy disks = 18*/

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t FStype);
void FAT_HANDLER(uint32_t *drv);

/* the indentifier for drivers + information about our driver */
struct DRIVER FAT_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_FAT | DRIVER_TYPE_FS), (uint32_t) (FAT_HANDLER)};

/* FAT12 and 16 COMING SOON */

void FAT_HANDLER(uint32_t *drv)
{
    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
        FAT_init(drv[1], drv[2], drv[3]);
        break;
    }
}

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t FStype)
{
    uint32_t startLBA = MBR_getStartLBA((uint8_t) drive, (uint8_t) partition);

    /* is the partition defined already? */
    if(partition_info_t[drive].FStype)
        return;

    partition_info_t[partition].drive = (uint8_t) drive;
    partition_info_t[drive].FStype    = (uint8_t) FStype;
    /* todo:
      - save drive to info list (don't forget to check if we've already saved it)
      - detect fat type
      - save fat type to info list
      - get all necessarry info about the fs
      - print hello -- ifndef NO_DEBUG_INFO of course */

}
