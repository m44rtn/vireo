#ifndef FAT_H
#define FAT_H

#include "../ATA/ATA.h"
#include "../../io/util.h"
#include "../../io/memory.h"
#include "../../io/hardware.h"
#include "../../include/error.h"
#include "../../include/types.h"
#include "../../include/DEFATA.h"
#include "../../include/VFS.h"


typedef struct{
    char bootstuff[3];
    uint8_t OEM[8];
    uint16_t bSector;
    uint8_t SectClust;
    uint16_t resvSect;
    uint8_t nFAT;
    uint16_t nDirEnt;
    uint16_t nSect;
    uint8_t MDT; //media descriptor type
    uint16_t sectFAT;
    uint16_t SectTrack;
    uint16_t HeadsTrack;
    uint32_t HiddenSect;
    uint32_t lnSect;

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
} __attribute__ ((packed)) FAT32_BPB;

typedef struct{
    char name[8];
    char ext[3]; //file extension
    uint8_t attrib;
    uint8_t uAttrib; //user attributes

    char undelete;
    uint16_t cTime; //create time
    uint16_t cDate; //create date
    uint16_t aDate; //access date
    uint16_t clHi; //cluster high

    uint16_t mTime; //modified time
    uint16_t mDate; //modified date
    uint16_t clLo; //cluster low
    uint32_t fSize; //file size
} __attribute__ ((packed)) FAT32_DIR;

typedef struct
{
    uint32_t FileLoc;
    uint32_t size;
} File;

FAT32_BPB *BPB;

uint32_t *GetFile(char *filename, uint8_t drive, uint32_t dirLoc);
File *FindFile(char *filename, uint32_t dirLoc, uint8_t drive);

FAT32_BPB *GetBootCodeFAT(uint8_t drive);
FAT32_BPB *FATinit(uint8_t drive);

uint32_t FAT_Traverse(char *name);

uint32_t FindNextDir(char *DirName,uint8_t drive, uint32_t prevClust);

void FAT32_WRITE_FILE(uint8_t drive, uint32_t *file, size_t size, char *name, char *path);
FAT32_DIR *FAT32_DIRPREP(uint8_t drive, FAT32_DIR *dir, uint32_t clust, size_t size, uint32_t len, char *name);
void FAT32_Write_cluster(uint8_t drive, uint32_t cluster, uint32_t *buf);

uint8_t *FAT32_READ_FILE(uint8_t drive, uint32_t cluster, size_t size);
uint32_t FAT_cluster_LBA(uint32_t cluster);
tVFS *FAT_read_table(uint8_t drive, uint32_t cluster, uint32_t *nClusts);
FAT32_DIR *ReadDir(uint8_t drive, uint32_t cluster, uint32_t *len);

tVFS *FATFindFreeClusterChain(uint8_t drive, uint32_t size, uint32_t *len);

#endif