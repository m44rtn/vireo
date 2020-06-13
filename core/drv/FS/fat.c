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
#include "../../include/exit_code.h"
#include "../../include/file.h"

#include "../../io/diskio.h"

#include "../../memory/memory.h"

#include "../../hardware/driver.h"

#include "../../kernel/mbr.h"

#include "../../screen/screen_basic.h"

#include "../../dbg/dbg.h"

#include "../../util/util.h"

#include "../COMMANDS.H"
#include "../FS_TYPES.H"
#include "../FS_commands.h"

#define FAT_FS_ID32 0x0B

#define FAT_MAX_PARTITIONS 8 /* if you want to use more than 8 partitions, then feel free to change this number */

#define FAT_DIR_ATTRIB_READ_ONLY    0x01
#define FAT_DIR_ATTRIB_HIDDEN       0x02
#define FAT_DIR_ATTRIB_SYSTEM       0x04
#define FAT_DIR_ATTRIB_VOLUME_ID    0x08
#define FAT_DIR_ATTRIB_DIRECTORY    0x10
#define FAT_DIR_ATTRIB_ARCHIVE      0x20
#define FAT_DIR_ATTRIB_LFN          0x3F /* every possible attrib OR'ed */


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
    uint8_t partition;
    uint8_t FStype;

    char volname[12];

    uint8_t sectclust;
    uint32_t fatsector;
    uint32_t rootcluster;  
    uint32_t firstdatasect;  

    /* maybe make a seperate FS_INFO for FAT12/16? */
} __attribute__((packed)) FS_INFO;


void FAT_HANDLER(uint32_t *drv);

static uint32_t FAT_cluster_LBA(uint32_t cluster);
static FS_INFO *FAT_getInfo(void);
static void FAT_init(uint32_t drive, uint32_t partition, uint32_t FStype);
static uint8_t FAT_alreadyExists(uint8_t drive, uint8_t partition);
static uint32_t *FAT32_readFat(uint32_t cluster, uint32_t *nclusters);
static uint16_t *FAT32_readDir(uint32_t cluster);
static uint32_t FAT32_getClusterByFileName(FAT32_DIR *dir, char *filename);

static uint32_t FAT_convertPath(char *path);
void FAT_convertFilename(char *path, char *dummy);

static void test_dir(void);


static uint32_t *partition_info_t[FAT_MAX_PARTITIONS];
static uint8_t partitions = 0;
static uint8_t currentWorkingPartition, currentWorkingDrive;
static uint8_t gErrorCode = 0;
static uint32_t *info_buffer;

/* the indentifier for drivers + information about our driver */
struct DRIVER FAT_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_FAT32 | DRIVER_TYPE_FS), (uint32_t) (FAT_HANDLER)};

/* FAT12 and 16 COMING SOON */

void FAT_HANDLER(uint32_t *drv)
{

    uint32_t cluster;
    uint32_t *ptr;
    char *dummy;
    
    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
        {
            if(!info_buffer)
                info_buffer = (uint32_t *) kmalloc(512);
            FAT_init(drv[1], drv[2], drv[3]);
        break;
        }

        case FS_COMMAND_READ:
            cluster = FAT_convertPath((char *)drv[1]);
            

            if(cluster == EXIT_CODE32_FAT_FAIL)
            {
                drv[4] = gErrorCode;
                break;
            }
            
            if(gErrorCode == EXIT_CODE_FAT_UNKNOWN_DIR)
            {
                /*ptr = FAT_readFile(FAT_convertFilename((char *) drv[1]), cluster);*/
                FAT_convertFilenameToFATCompat(drv[1], dummy);
                /*drv[2] = (uint32_t) vmalloc(size, pid, 1);*/
                /*ptr = FAT_readFile(dummy, cluster);*/
            }
            else
                ptr = (uint32_t *) FAT32_readDir(cluster);
            
            
        break;
    }
    
    gErrorCode = EXIT_CODE_GLOBAL_SUCCESS;
}

static uint32_t FAT_cluster_LBA(uint32_t cluster)
{
    FS_INFO *info = FAT_getInfo();

    uint32_t LBA = ((cluster - 2) * info->sectclust) + info->firstdatasect;
    return LBA;
}

static FS_INFO *FAT_getInfo(void)
{
    uint8_t i;
    FS_INFO *info;

    for(i = 0; i < FAT_MAX_PARTITIONS; ++i)
    {
        info = (FS_INFO *) partition_info_t[i];
        
        if(info->drive == currentWorkingDrive 
            && info->partition == currentWorkingPartition)
            break;
    }

    dbg_assert(!(i == FAT_MAX_PARTITIONS));

    return info;
}

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t FStype)
{
    uint8_t error;
    uint32_t startLBA = MBR_getStartLBA((uint8_t) drive, (uint8_t) partition);
    uint8_t *buf         = kmalloc(512);
    FS_INFO *info     = (FS_INFO *) (((uint32_t)info_buffer) + (partitions*sizeof(FS_INFO)));
    FAT32_EBPB *bpb  = (FAT32_EBPB *) buf; 

    /* if we already know this partition then... what are you doing here? */
    if(FAT_alreadyExists((uint8_t)drive, (uint8_t)partition))
        return;
    
    /* read the BPB */
    error = READ((uint8_t)drive, startLBA, 1U, buf);
    if(error)
        return;

    /* only support 512 bytes per sector */
    dbg_assert(bpb->bpb.bSector == 512U);

    info->fatsector = bpb->bpb.resvSect + startLBA;

    /* store the cluster for the root directory */
    if(FStype == FS_TYPE_FAT32)
        info->rootcluster = bpb->clustLocRootdir;
    else if(FStype == FS_TYPE_FAT12 || FStype == FS_TYPE_FAT16)
        return; /* TODO: not implemented */
    
    memcpy((char *)&info->volname, (char *)&bpb->volName, 11);
    info->volname[11] = '\0';

    info->firstdatasect = info->fatsector + (bpb->bpb.nFAT * bpb->sectFAT32);
    info->sectclust     = bpb->bpb.SectClust;
    
    #ifndef NO_DEBUG_INFO
    trace((char *)"[FAT_DRIVER] Volume name: %s\n", (uint32_t) &info->volname);
    trace((char *)"[FAT_DRIVER] FAT cluster: %i\n", (uint32_t) info->fatsector);
    trace((char *)"[FAT_DRIVER] RootDir cluster: %i\n", (uint32_t) info->rootcluster);
    trace((char *)"[FAT_DRIVER] FS_INFO *: 0x%x\n", (uint32_t) info);
    print((char *)"\n");
    #endif

    info->FStype = (uint8_t) (FStype);

    partition_info_t[partitions] = (uint32_t *) info;
    ++partitions;
    
    kfree(buf);
}

/* returns fail when partition exists in our list, success when does not exist */
static uint8_t FAT_alreadyExists(uint8_t drive, uint8_t partition)
{   
    uint8_t i;
    FS_INFO *info;

    for(i = 0; i < FAT_MAX_PARTITIONS; ++i)
    {
        /* if the pointer is NULL we have no information for that partition */
        if(partition_info_t[i] == NULL)
            continue;

        info = (FS_INFO *) partition_info_t[i];

        if(!(info->drive == drive && info->partition == partition))
            continue;

        /* FStype can never be zero if the info block is initialized */
        if(info->FStype)
            return EXIT_CODE_GLOBAL_GENERAL_FAIL;            
    }

    return EXIT_CODE_GLOBAL_SUCCESS;
}

/* returns succes when succesful, not supported when the id is an unknown format */
static uint8_t FAT_setDrivePartitionActive(const char *id)
{
    uint8_t drive, partition;

    /* get the drive number  
        HD0 (where HD means harddrive and the 0 is the drive number) */
    drive = strdigit_toInt(id[2]);
    
    if(drive == EXIT_CODE_GLOBAL_UNSUPPORTED)
        return EXIT_CODE_FAT_UNSUPPORTED_DRIVE;

    /* get the partition number
        P0 (where P means partition and the 0 is the partition number) */
    partition = strdigit_toInt(id[4]);

    if(partition == EXIT_CODE_GLOBAL_UNSUPPORTED)
        return EXIT_CODE_FAT_UNSUPPORTED_DRIVE;

    /* set the current working drive and partititon */
    currentWorkingDrive = drive;
    currentWorkingPartition = partition;

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static uint32_t *FAT32_readFat(uint32_t cluster, uint32_t *nclusters)
{
    uint8_t error;
    FS_INFO *info      = FAT_getInfo();
    uint32_t *table    = kmalloc(512);
    uint32_t *clusters[128];
    uint32_t fat_sector, entry;

    *(nclusters) = 0;
    
    while(cluster != 0 && ((cluster & 0x0FFFFFFF) < 0x0FFFFFF8))
    {
        /* store the cluster found in the previous round of this loop 
        (if we just now started, this will store the cluster given to us as
        an argument) */
        clusters[*(nclusters)] = cluster;

        /* what do we need to read? */        
        fat_sector = (uint32_t) info->fatsector + ((cluster * 4) / 512);
        entry = (cluster * 4) % 512;

        /* read it! */
        error = READ(info->drive, fat_sector, 1U, (uint8_t *) table);
        
        if(error)
            return NULL;

        cluster = *(&table[entry]) & 0x0FFFFFFF;

        *(nclusters) = *(nclusters) + 1U;
    }

    kfree(table);
    
    return &clusters;
}

static uint16_t *FAT32_readDir(uint32_t cluster)
{
    FS_INFO *info = FAT_getInfo();
    uint32_t nclusters;
    uint32_t *dir_clusters = FAT32_readFat((cluster) ? cluster : info->rootcluster, &nclusters); 
    uint16_t *buffer = (uint16_t *) kmalloc(nclusters * info->sectclust * 512);
    uint16_t *original = (uint16_t *) buffer;
    uint32_t lba, i;
    uint8_t error;

    for(i = 0; i < 1; ++i)
    {
        lba = FAT_cluster_LBA(cluster);
        
        error = READ(0, lba, 1, (uint8_t *) buffer);
        if(error)
            return NULL;
        
        buffer = (uint16_t *) (((uint32_t)buffer) + 512);
    }

    
    kfree(dir_clusters);
    
    return original;
}

/* function assumes filename to be FAT compatible (no dots, 11 chars long) */
static uint32_t FAT32_getClusterByFileName(FAT32_DIR *dir, char *filename)
{
    uint32_t i = 0;
    char file[12];

    /* TODO: make me work with dots for filenames like in (hi.txt) */

    file[11] = '\0';

    while(dir[i].name[0] && (i != 0xFFFFFFFF))
    {
        /* not a dir, not a file? skip! (also skip long filenames, who cares 
        about them, right? pff! --> going full DOS style 
        here with the great, nice, lovely 11 character filenames) :) */
        if(dir[i].name[0] == 0xE5 || dir[i].attrib == FAT_DIR_ATTRIB_LFN || 
            dir[i].attrib == FAT_DIR_ATTRIB_VOLUME_ID || 
            dir[i].attrib == FAT_DIR_ATTRIB_HIDDEN)
            {
                ++i;
                continue;
            }
        
        memcpy((char *)&file, (char *)&dir[i].name, 8);
        memcpy((char *)&file[8], (char *)&dir[i].ext, 3);
        
        /* return the cluster of the file- or directoryname */
        if(!strcmp(&file, filename))
            return (uint32_t) (dir[i].clHi << 16) | dir[i].clLo;
        
        ++i;
    }

    dbg_assert(!(i >= EXIT_CODE32_FAT_FAIL));
    return EXIT_CODE32_FAT_FAIL;
    
}

/* if an error is detected this will return 0xFFFFFFFF (since a sector could never
have that value) and the first byte of path, will be the official error code */
static uint32_t FAT_convertPath(char *path)
{
    uint8_t stat = 0;
    char *current, *backup;
    uint32_t cluster = 2, prevCluster = 0;
    FAT32_DIR *dir = (FAT32_DIR *) NULL;
    char filename[12];
    uint32_t i = 0;

    memcpy(backup, path, strlen(path));
    
    current = strtok(backup, "/");
    gErrorCode = stat = FAT_setDrivePartitionActive(current);


    if(stat == EXIT_CODE_FAT_UNSUPPORTED_DRIVE)
        return EXIT_CODE32_FAT_FAIL; /* this is a 32-bit value!!! */

    while(current = strtok(NULL, "/"))
    {
        dir = (FAT32_DIR *) FAT32_readDir(cluster);
        prevCluster = cluster;
        cluster = FAT32_getClusterByFileName(dir, current);

        /* we either found a file or we got a non-existing dir */
        if(cluster == EXIT_CODE32_FAT_FAIL)
        {
            gErrorCode = EXIT_CODE_FAT_UNKNOWN_DIR;
            cluster = prevCluster;
            break;
        }

        kfree(dir);
    }
    
    return cluster;
}

/* This function converts a filename 'file.txt' to 'file    txt'
    (aka FAT compatible)

    the argument dummy is where the new FAT compatible filename will live */
void FAT_convertFilenameToFATCompat(char *path, char *dummy)
{
    char *current, *prev, *file;
    uint8_t spaces, i;
    char filename[12];
    size_t len;
    
    memcpy(file, path, strlen(path));

    current = strtok(file, "/");
    prev = current;

    /* since we get the entire file path, we should get rid of the path itself */
    while(current = strtok(NULL, "/"))
        prev = current;

    trace("prev: %s\n", prev);

    /* we got rid of the file path and we now just have the filename!
     so let's get rid of the dot and copy the filename itself */
    current = strtok(prev, ".");
    len = (uint8_t) strlen(current);
    memcpy((char *) &filename, (char *) current, len);
    
    /* add the right amount of spaces as padding */
    spaces = 8U - len;
    for(i = 0; i < spaces; ++i)
        filename[len+i] = ' ';
    
    /* now let's get working on the extension */
    current = strtok(NULL, ".");
    if(current != NULL)
    {
        /* if we get here the file DID have an extension, so let's copy that */
        len = strlen(current);
        memcpy((char *) &filename[8], (char *) current, len);
    }
    else
        /* no file extension, so make sure we only put spaces there */
        len = 0;
    
    /* now add the right amount of spaces as padding */
    spaces = 3U - len;
    for(i = 0; i < spaces; ++i)
        filename[8+len+i] = ' ';
    
    /* let's null terminate the string, because we can */
    filename[11] = '\0';
    
    /* put the entie FAT compatible filename in the dummy given 
    as argument tot this function */
    memcpy(dummy, &filename, 12);
}

/* this is just for testing, this *should* have been removed before you
saw this... if you can read this, I forgot :/ */
static void test_dir(void)
{
    uint32_t clusterssss;
    uint16_t *buf = (uint16_t *) FAT32_readDir(2);
    FAT32_DIR *dir = (FAT32_DIR *) buf;
    uint32_t i = 0;
    char filename[12];
    
    filename[11] = '\0';

    while(dir[i].name[0])
    {
        if(dir[i].name[0] == 0xE5 || dir[i].attrib == FAT_DIR_ATTRIB_LFN || dir[i].attrib == FAT_DIR_ATTRIB_VOLUME_ID || dir[i].attrib == FAT_DIR_ATTRIB_HIDDEN)
            {++i;continue;}
        
        memcpy((char *)&filename, (char *)&dir[i].name, 8);
        memcpy((char *)&filename[8], (char *)&dir[i].ext, 3);
        trace("%s\t", &filename);
        trace("%x\n", dir[i].attrib);
        ++i;
    }
    
    kfree((void *)buf);
}