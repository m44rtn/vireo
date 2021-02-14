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

#include "fat.h"

#include "../../include/types.h"
#include "../../include/diskstuff.h"
#include "../../include/exit_code.h"
#include "../../include/file.h"

#include "../../io/diskio.h"

#include "../../memory/memory.h"
#include "../../memory/paging.h"

#include "../../hardware/driver.h"

#include "../../kernel/mbr.h"

#include "../../screen/screen_basic.h"

#include "../../dbg/dbg.h"

#include "../../util/util.h"

#include "../../kernel/panic.h"

#include "../COMMANDS.H"
#include "../FS_TYPES.H"
#include "../FS_commands.h"

#define FAT_FS_ID32 0x0B

#define FAT_MAX_PARTITIONS          8 /* if you want to use more than 8 partitions, then feel free to change this number */
#define FAT_SUPPORTED_DRIVES        2 /* this driver is stupid, please choose another one. */
#define FAT_PARTITIONS_PER_DRIVE    4 /* I'm really dissapointed in this driver */

#define SECTOR_SIZE     512U /* bytes */

#define FAT_DIR_ATTRIB_READ_ONLY    0x01
#define FAT_DIR_ATTRIB_HIDDEN       0x02
#define FAT_DIR_ATTRIB_SYSTEM       0x04
#define FAT_DIR_ATTRIB_VOLUME_ID    0x08
#define FAT_DIR_ATTRIB_DIRECTORY    0x10
#define FAT_DIR_ATTRIB_ARCHIVE      0x20
#define FAT_DIR_ATTRIB_LFN          0x3F /* every possible attrib OR'ed */

#define FAT_DRIVER_FLAG_INIT_RAN    1 /* init has ran succesfully before */

#define FILENAME_LEN                11


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
    uint16_t fat_flags;
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
    uint8_t uAttrib; // unused/reserved for WinNT

    char undelete;  // creation time in tenths of a second
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

    // FIXME: make linked list?
    // perhaps a linked list would be better for flexibillity and maintenance 
    // ...and maybe also to make me feel less dissapointed by my work on this driver
} __attribute__((packed)) FS_INFO;


// functions
void FAT_HANDLER(uint32_t *drv);

static void FAT_setup_FS_INFO(void);

static uint32_t FAT_cluster_LBA(uint32_t cluster);

static FS_INFO * FAT_getInfo(void);

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t FStype);
static uint8_t FAT_alreadyExists(uint8_t drive, uint8_t partition);

static void FAT_get_file_name(char *path, char *output);
static void FAT_rename(char *old, char *new);

static uint32_t *FAT32_readFat(uint32_t cluster, uint32_t *nclusters);
static uint32_t FAT32_read_table(uint32_t cluster);

static uint16_t *FAT32_readDir(uint32_t cluster);
static FAT32_DIR *FAT_find_in_dir(FAT32_DIR *dir, char *filename);
static uint32_t FAT_file_exists(FAT32_DIR *dir, char *filename);

static uint8_t FAT_setDrivePartitionActive(const char *id);

static uint32_t *FAT32_read_file(FAT32_DIR *entry);
static void FAT_save_file(char *filename, uint16_t * buffer, size_t buffer_size, uint8_t attrib);

static void save(uint32_t *clusters, uint32_t n, uint16_t *buffer, size_t buffer_size);
static uint32_t * seek_empty_clusters(uint32_t n);
static uint8_t update_dir(char *path, size_t fsize, uint8_t attrib, uint32_t fcluster);
static void update_fat(uint32_t *clusters, uint32_t n);

static uint32_t dir_find_last(FAT32_DIR *dir);
static uint32_t dir_prepare_new(FAT32_DIR *dir, uint32_t *entry, FS_INFO *info, uint32_t original_cluster);
static uint32_t dir_determine_cluster(uint32_t entry, uint32_t cluster, FS_INFO *info);

static uint32_t path_find_last(char *str);

static void FAT_convertFilenameToFATCompat(char *path, char *dummy);
static FAT32_DIR *FAT_convertPath(char *path, uint32_t *dir_cluster);

static volatile FS_INFO partition_info_t[FAT_MAX_PARTITIONS];
static volatile uint32_t *partition_info_ptr = NULL;
static volatile uint8_t currentWorkingPartition, currentWorkingDrive;
static volatile uint8_t gErrorCode = 0;
static volatile uint32_t *info_buffer; /* location of BPB in memory */

/* the indentifier for drivers + information about our driver */
struct DRIVER FAT_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_FAT32 | DRIVER_TYPE_FS), (uint32_t) (FAT_HANDLER)};

volatile uint16_t fat_flags = 0;

/* FAT12 and 16 come sometime in the future */

// TODO: cleanup and delete, I'm sure there are memory leaks somewhere
// READ doesn't work after a write, or at least not immidiately after a write

void FAT_HANDLER(uint32_t *drv)
{
    FAT32_DIR *dir_entry;
    uint32_t *ptr;
    
    gErrorCode = EXIT_CODE_GLOBAL_SUCCESS;
    
    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
            currentWorkingDrive = (uint8_t) drv[1];
            currentWorkingPartition = (uint8_t) drv[2];

            /* FIXME: this driver seems to only support one drive, thanks to this... */
            if(!info_buffer)
                info_buffer = (uint32_t *) kmalloc(SECTOR_SIZE); /* boot sector */

            if(!(fat_flags && FAT_DRIVER_FLAG_INIT_RAN))
                FAT_setup_FS_INFO();

            FAT_init(currentWorkingDrive, currentWorkingPartition, drv[3]);
        break;

        case FS_COMMAND_READ:
            gErrorCode = FAT_setDrivePartitionActive((const char *) drv[1]);

            if(gErrorCode == EXIT_CODE_FAT_UNSUPPORTED_DRIVE)
                break;

            /* starting cluster of requested item (DIR or FILE) */
            uint32_t ignore;
            dir_entry = FAT_convertPath((char *)drv[1], &ignore);

            if(dir_entry == NULL)
            {
                gErrorCode = EXIT_CODE_FAT_FILE_NOT_FOUND;
                break;
            }

            // TODO: test file not found error
            ptr = FAT32_read_file(dir_entry);

            if(ptr == NULL)
                gErrorCode = EXIT_CODE_GLOBAL_GENERAL_FAIL;

            drv[2] = (uint32_t) ptr;
            drv[3] = dir_entry->fSize;
        
        break;

        case FS_COMMAND_WRITE:
            gErrorCode = FAT_setDrivePartitionActive((const char *) drv[1]);

            if(gErrorCode == EXIT_CODE_FAT_UNSUPPORTED_DRIVE)
                break;

            FAT_save_file((char *) drv[1], (uint16_t *) drv[2], (size_t) drv[3], (uint8_t) drv[4]);      
        break;

        case FS_COMMAND_RENAME:
            gErrorCode = FAT_setDrivePartitionActive((const char *) drv[1]);

            if(gErrorCode == EXIT_CODE_FAT_UNSUPPORTED_DRIVE)
                break;

            FAT_rename((char *) drv[1], (char *) drv[2]);      
        break;

        default:
            gErrorCode = EXIT_CODE_FAT_UNSUPPORTED_COMMAND;
        break;
        
    }
    
    drv[4] = gErrorCode;
}

static void FAT_setup_FS_INFO(void)
{
    uint8_t drive = 0;
    for(uint8_t i = 0; i < FAT_MAX_PARTITIONS >> 1; ++i)
    {
       /* first drive */
       partition_info_t[i].drive = (uint8_t) ((i >> 2U) - 1U);
       partition_info_t[i].partition = (uint8_t) (i - ((drive + 1U) * 2U));
    }
}

static uint32_t FAT_cluster_LBA(uint32_t cluster)
{
    FS_INFO* info = FAT_getInfo();

    uint32_t LBA = ((cluster - 2) * (info->sectclust)) + (info->firstdatasect);
    return LBA;
}

/* retrieves information structure */
static FS_INFO *FAT_getInfo(void)
{
    // TODO: could be a macro
    //dbg_assert(partition_info_ptr);
    return (FS_INFO *) partition_info_ptr; //(&partition_info_t[(currentWorkingDrive + 1) * currentWorkingPartition]);
}

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t FStype)
{
    uint8_t error;
    uint32_t startLBA   = MBR_getStartLBA((uint8_t) drive, (uint8_t) partition);
    uint8_t *buf        = kmalloc(512);
    FS_INFO *info       = (FS_INFO *) &partition_info_t;
    FAT32_EBPB *bpb     = (FAT32_EBPB *) buf; 

    /* if we already know this partition then... what are you doing here? */
    if(FAT_alreadyExists((uint8_t)drive, (uint8_t)partition))
        return;
    
    /* read the BPB */
    error = READ((uint8_t)drive, startLBA, 1U, buf);
    if(error)
        return;

    /* only support 512 bytes per sector */
    dbg_assert(bpb->bpb.bSector == 512U);
    info->fatsector = (bpb->bpb.resvSect) + startLBA;


    /* store the cluster for the root directory */
    if(FStype == FS_TYPE_FAT32)
        info->rootcluster = bpb->clustLocRootdir;
    else if(FStype == FS_TYPE_FAT12 || FStype == FS_TYPE_FAT16)
        return; /* TODO: not implemented */
    
    memcpy((char *)&info->volname, (char *)&bpb->volName, 11);
    info->volname[11] = '\0';

    info->firstdatasect = (info->fatsector) + (bpb->bpb.nFAT * bpb->sectFAT32);
    info->sectclust     = bpb->bpb.SectClust;

    partition_info_ptr = (uint32_t *) info;
    
    #ifndef NO_DEBUG_INFO
    trace("[FAT_DRIVER] Drive: %i\n", drive);
    trace("[FAT_DRIVER] Partition: %i\n", partition);
    trace("[FAT_DRIVER] Volume name: %s\n", (uint32_t) &info->volname);
    trace("[FAT_DRIVER] FAT cluster: %i\n", (uint32_t) info->fatsector);
    trace("[FAT_DRIVER] RootDir cluster: %i\n", (uint32_t) info->rootcluster);
    trace("[FAT_DRIVER] FS_INFO * *: 0x%x\n", (uint32_t) info);
    print("\n");
    #endif

    info->FStype = (uint8_t) (FStype);
    
    kfree(buf);

    fat_flags |= FAT_DRIVER_FLAG_INIT_RAN;
}

/* returns fail when partition exists in our list, success when does not exist */
static uint8_t FAT_alreadyExists(uint8_t drive, uint8_t partition)
{   
    uint8_t i;
    FS_INFO * info = FAT_getInfo();

    for(i = 0; i < FAT_MAX_PARTITIONS; ++i)
    {
        /* if the pointer is NULL we have no information for that partition */

        if(!((info->drive) == drive && (info->partition) == partition))
            continue;

        /* FStype can never be zero if the info block is initialized 
            so, one that isn't zero means we've already had this one... */
        if(info->FStype)
            return EXIT_CODE_GLOBAL_GENERAL_FAIL;            
    }

    return EXIT_CODE_GLOBAL_SUCCESS;
}

/* returns succes when succesful, returns not supported when the id is an unknown format */
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
    uint32_t *clusters = kmalloc(128 * sizeof(uint32_t));

    // store starting cluster, since it's part of the chain
    clusters[0] = cluster;
    
    *(nclusters) = 1;
    uint32_t i = 1;

    while(cluster != 0 && ((cluster & 0x0FFFFFFF) < 0x0FFFFFF8))
    {
        /* store the cluster found in the previous round of this loop 
        (if we just now started, this will store the cluster given to us as
        an argument) */
        clusters[(uint32_t) *(nclusters)] = cluster;

        if(cluster == NULL)
            return NULL;

        cluster = FAT32_read_table(cluster);

        i++;
        *(nclusters) = i;
    }

    return clusters;
}

static uint32_t FAT32_read_table(uint32_t cluster)
{
    uint8_t error;
    FS_INFO *info     = FAT_getInfo();
    uint32_t fat_sector, entry;

    uint32_t *table    = kmalloc(512);

    /* what do we need to read? */        
    fat_sector = (uint32_t) (info->fatsector) + ((cluster * 4) / 512);
    entry = (cluster * 4) % 512;

    /* read it! */
    error = READ(currentWorkingDrive, fat_sector, 1U, (uint8_t *) table); // TODO: TEST THIS!!!

    if(error)
        return NULL;

    uint32_t c = *(&table[entry]) & 0x0FFFFFFF;
   
    kfree(table);

    return c;
}

// returns directory
// TODO: maybe return FAT32_DIR?
static uint16_t *FAT32_readDir(uint32_t cluster)
{
    FS_INFO * info = FAT_getInfo();
    uint32_t nclusters = 0;

    cluster = (cluster) ? cluster : info->rootcluster;
    
    uint32_t *dir_clusters = FAT32_readFat(cluster, &nclusters); 
 
    uint16_t *buffer = (uint16_t *) kmalloc(nclusters * (info->sectclust) * 512);
    uint16_t *original = (uint16_t *) buffer;
    uint32_t lba, i;
    uint8_t error;

    for(i = 0; i < nclusters; ++i)
    {
        cluster = dir_clusters[i];
        lba = FAT_cluster_LBA(dir_clusters[i]);
        
        error = READ(currentWorkingDrive, lba, (info->sectclust), (uint8_t *) buffer);

        if(error)
            return NULL;     
        
        buffer = (uint16_t *) (((uint32_t)buffer) + 512);
    }

    kfree(dir_clusters);
    
    return original;
}

/* returns vptr of the buffer, or null if error*/
static uint32_t *FAT32_read_file(FAT32_DIR *entry)
{
    /* actually, if you're requesting a dir then the dir has already been read before (and has been erased from memory),
        in order to find it. believe it or not, it's double the work if we're going to read it again here.
        so, in the future that could be an optimalization TODO */

    /* get the clusters for the file from the FAT table */
    uint32_t cluster = (uint32_t) (entry->clHi << 16U) | (entry->clLo);
    uint32_t nclusters = 0;
    uint32_t *clusters = FAT32_readFat(cluster, &nclusters);

    FAT32_EBPB *bpb = (FAT32_EBPB *) info_buffer;

    const uint32_t sectclust = bpb->bpb.SectClust;

    /* file size and the number of lba's to be read */
    uint32_t fsize = (entry->fSize);

    // fixes overwriting everything when something goes wrong
    // if(fsize == MAX || fsize == 0)
    //     return NULL;

    uint32_t nlba_read  = (fsize >> 9) /* DIV by SECTOR_SIZE */ + 1U;

    uint32_t page_size = nlba_read << 9; /* bytes */

    /* request a page */
    PAGE_REQ *req = kmalloc(sizeof(PAGE_REQ));
    req->pid = PID_KERNEL;
    req->attr = PAGE_REQ_ATTR_READ_ONLY | PAGE_REQ_ATTR_SUPERVISOR;
    req->size = page_size;
    
    uint32_t *obuffer;
    uint32_t *buffer = obuffer = (uint32_t *) valloc(req);

    if(buffer == NULL)
        return NULL;

    kfree(req);
    trace("buffer: 0x%x\n", buffer);
    trace("clusters: 0x%x\n", clusters);

    /* read the clusters */
    for(uint32_t i = 0; i < nclusters; ++i)
    {
        const uint32_t nsects = (nlba_read < sectclust) ? nlba_read : sectclust;
        uint32_t starting_lba = FAT_cluster_LBA(clusters[i]);

        READ(currentWorkingDrive, starting_lba, (uint32_t) nsects, (uint8_t *) buffer);

        buffer += sectclust << 9;

        /* no underflows in my kingdom >:) */
        if(nlba_read >= sectclust)
            nlba_read -= sectclust;
    }
    kfree(clusters);
    kfree(entry);
    
    return obuffer;
}

static void FAT_get_file_name(char *path, char *output)
{
    uint32_t i = path_find_last(path);

    char *filename = kmalloc(12); //[12];
    memcpy(&filename[0], &path[i], strlen(&path[i]));

    FAT_convertFilenameToFATCompat(&filename[0], output);

    trace("filename: %s\n", output);
    kfree(filename);
}

static void FAT_rename(char *old, char *new)
{
    // FIXME: shares way too much code with update_dir
    // TODO: make this work
    FS_INFO *info = FAT_getInfo();

    char n[12], o[12], *obackup;
    
    // copy filenames and make FAT compatible
    FAT_get_file_name(old, &o[0]);
    FAT_convertFilenameToFATCompat(new, &n[0]);

    // backup path
    obackup = kmalloc(strlen(old) + 1);
    memcpy(obackup, old, strlen(old)+1);
    
    // find the last folder in the path and end the string there
    uint32_t i = path_find_last(obackup);
    obackup[i-1] = '\0';

    // find that cluster
    uint32_t cluster = 0;
    FAT32_DIR *dir = FAT_convertPath(obackup, &cluster); 

    uint32_t *d = (uint32_t *) FAT32_readDir(cluster); // was read file

    if(d == NULL)
    {
        print("yooo\n");
        gErrorCode = EXIT_CODE_FAT_FILE_NOT_FOUND;
        return;
    }
    kfree((void *) dir);

    dir = (FAT32_DIR *) d;

    trace("cluster: %x\n", cluster);
    trace("obackup: %s\n", obackup);

    if(FAT_file_exists(dir, &n[0]) != MAX)
    {
        print("yooo\n");
        gErrorCode = EXIT_CODE_FAT_FILE_EXISTS;
        return;
    }
    
    // find the file
    uint32_t entry = FAT_file_exists(dir, &o[0]);

    trace("entry: %x\n", entry);

    if(entry == MAX)
        return; // FIXME: kfree(dir) here

    memcpy((char *) &(dir[entry].name[0]), &n[0], 11U);

    cluster = dir_determine_cluster(entry, cluster, info);

    trace("cluster: 0x%x\n", cluster);

    // fixme: make this function save_dir()
    uint32_t c[1];
    c[0] = cluster;

    // trace("dir: 0x%x\n", d);
    // trace("entry: 0x%x\n", entry);
    // trace("&c: 0x%x\n\n\n\n\n\n\n\n\n", &c);

    // calculate the first entry which needs to be written to the cluster
    uint32_t start = (entry / (SECTOR_SIZE * (info->sectclust)));

    save(&c[0], 1U, (uint16_t *) &dir[start], SECTOR_SIZE * (info->sectclust)); 
    kfree(dir);
}

static void FAT_save_file(char *filename, uint16_t * buffer, size_t buffer_size, uint8_t attrib)
{
    FS_INFO *info = FAT_getInfo();

    print("\n\n\n\n==================\nWRITE FILE\n==================\n\n");
    
    // number of clusters necessarry to store the file
    uint32_t n_clusters = buffer_size / ((info->sectclust) * SECTOR_SIZE);
    n_clusters += (buffer_size % ((info->sectclust) * SECTOR_SIZE)) >= 1; // need an extra cluster?

    // seek empty clusters for the file
    uint32_t *clusters = seek_empty_clusters(1U); // first cluster to use

    //while(1);

     // update the directory listing, if something goes wrong it'll be here
    uint8_t error = update_dir(filename, buffer_size, attrib, clusters[0]);

    if(error)
    {

        // TODO: update fat here to reset everything
        trace("[FAT_DRIVER] Function error %x\n", error);
        gErrorCode = error;
        return;
    }

    // you may ask, why go through all this trouble to find one cluster
    // at the start of the function and then afterwards search for a few more,
    // resulting in the need for copying memory and all that?
    // well... at the time of writing, I was lazy and didn't feel like
    // changing the update_fat() function to reset clusters when something went wrong above.
    // It would be necessary since otherwise I would have to update the fat before trying to
    // update the directory. Don't believe me? You should. (╯°□°）╯︵ ┻━┻ 
    // resetting, btw, would be even slower since you may have to use PIO multiple times.
    uint32_t *c = kmalloc(n_clusters * sizeof(uint32_t));
    c[0] = clusters[0];

    kfree(clusters);
    

    if(n_clusters-1U)
    {
        clusters = seek_empty_clusters(n_clusters-1U);
        memcpy((c + sizeof(uint32_t)), clusters, (n_clusters-1) * sizeof(uint32_t));
        kfree(clusters);
    }


    trace("n_clusters: %i\n", n_clusters);

    trace("svfile: filename: 0x%x\n", filename);
    trace("svfile: buffer: 0x%x\n", buffer);
    // while(1);

    // update the FAT to include the right clusters
    update_fat(&c[0], n_clusters);

    // now, save the file
    // FIXME: ALS DEZE UITGECOMMENT IS WERKT T ALLEMAAL OKAY, MAAR ALS
    // IE NIET UITGECOMMENT IS GAAT T ALLEMAAL NAAR ZN FLIKKER
    save(&c[0], n_clusters, buffer, buffer_size);

    // cleanup time!
    kfree(c);
}

static uint8_t update_dir(char *path, size_t fsize, uint8_t attrib, uint32_t fcluster)
{  
    // remove last item from the path string (can become one function)
    char *s = kmalloc(strlen(path));
    memcpy(s, path, strlen(path) + 1);

    uint32_t i = path_find_last(s);
    
    char f[12];
    FAT_get_file_name(s, &f[0]);
    
    s[i-1] = '\0';

    trace("s:::::%s\n", s);

    // FIXME: could be function get_dir or something
    uint32_t cluster = 0; //  we need to know what the first cluster of this directory is
    FAT32_DIR *dir = FAT_convertPath(&s[0], &cluster); 

    uint32_t *d = (uint32_t *) FAT32_readDir(cluster); // was read file

    if(d == NULL)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    kfree((void *) dir);

    dir = (FAT32_DIR *) d;

    trace("s: %s\n", s);
    trace("cluster:: %x\n", cluster);
    trace("dir: 0x%x\n", dir);
    trace("f: %s\n", &f[0]);

    // file exists?
    if(FAT_file_exists(dir, &f[0]) != MAX)
        return EXIT_CODE_FAT_FILE_EXISTS;

    uint32_t entry = dir_find_last(dir);
    FS_INFO *info = FAT_getInfo();

    // do we need a new cluster?
    if((entry * sizeof(FAT32_DIR)) >= (SECTOR_SIZE * (info->sectclust)))
        cluster = dir_prepare_new(dir, &entry, info, cluster);
    else // FIXME: pass the right buffer pointer based on which cluster we need to write to
        // determine the right cluster, this way may cause round-down issues later
        cluster = dir_determine_cluster(entry, cluster, info);

    // make the entry 
    dir[entry].attrib = attrib;
    dir[entry].clHi = (uint16_t) ((fcluster >> 16U) & 0xFFFFU);
    dir[entry].clLo = (uint16_t) (fcluster & 0xFFFFU);
    dir[entry].fSize = fsize;
    
    // save the name
    memcpy(&(dir[entry].name[0]), &f[0], 11);

    // FIXME: could become function save_dir()    
    // save it
    uint32_t c[1];
    c[0] = cluster;

    trace("root cluster: 0x%x\n", (info->rootcluster));
    trace("&c: 0x%x\n", &c);

    trace("filename: 0x%x\n", path);
    trace("s: 0x%x\n", s);
    trace("d: 0x%x\n", d);
    trace("dir: 0x%x\n", dir);

    // calculate the first entry which needs to be written to the cluster
    uint32_t start = (entry / (SECTOR_SIZE * (info->sectclust)));

    save(&c[0], 1, (uint16_t *) &dir[start], SECTOR_SIZE * (info->sectclust)); 

    trace("f2: %s\n", &f[0]);
    
    // TODO: also if error
    kfree(d);
    kfree(s);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static uint32_t dir_determine_cluster(uint32_t entry, uint32_t cluster, FS_INFO *info)
{
    uint32_t i = 0;
    uint32_t *clusters = FAT32_readFat(cluster, &i);

    // calculate at which cluster in the list the entry should be at
    uint32_t c = ((entry * sizeof(FAT32_DIR)) >> 9U /* DIV by SECTOR_SIZE */);
    c += (((entry * sizeof(FAT32_DIR)) % SECTOR_SIZE) != 0);
    c /= (info->sectclust);


    trace("&clusters: 0x%x\n", clusters);
    trace("i: 0x%x\n", i);
    trace("cluster is %x\n", clusters[i]);
    // while(1);

    if(clusters[c] == 0x0FFFFFFF)
        return cluster;

    // get and return cluster
    c = clusters[c];
    kfree(clusters);

    return c;
}
// returns new cluster
static uint32_t dir_prepare_new(FAT32_DIR *dir, uint32_t *entry, FS_INFO *info, uint32_t original_cluster)
{
    // free the current directory entry
    kfree(dir);
    *(entry) = 0;

    FAT32_DIR *d = (FAT32_DIR *) kmalloc((info->sectclust) * SECTOR_SIZE);
    *(&dir) = d;

    trace("dir: 0x%x\n", dir);
    trace("d: 0x%x\n", d);

    uint32_t *clusters = seek_empty_clusters(1U);


    // get the last cluster from the current directory
    uint32_t nclusters = 0;
    uint32_t *fat_clusters = FAT32_readFat(original_cluster, &nclusters);

    uint32_t c[2];
    c[1] = clusters[0];
    c[0] = fat_clusters[nclusters-1];

    kfree(clusters);

    update_fat(&c[0], 2);

    return c[1];    
}

static uint32_t dir_find_last(FAT32_DIR *dir)
{
    uint32_t n;

    // search for free entry
    for(n = 0; dir[n].name[0]; ++n);

    return n;
}

static uint32_t path_find_last(char *str)
{
	char *s = kmalloc(strlen(str) + 1);
	memcpy(s, str, strlen(str));

	char *current, prev[12];
    size_t size = 0;
	strtok(s, "/");

	while((current = strtok(NULL, "/")) != NULL)
        memcpy(&prev[0], current, size = strlen(current));
    
    prev[size] = '\0';
    uint32_t i = findstr(str, &prev[0]);
    
    return i;
}

static void update_fat(uint32_t *clusters, uint32_t n)
{
    uint8_t error;
    FS_INFO *info     = FAT_getInfo();
    uint32_t fat_sector, entry;

    uint32_t *table    = kmalloc(512);

    // TODO: allow clusters to be set to 0 meaning free

    trace("clusters: 0x%x\n", clusters);
    trace("n: %x\n", n);
    //while(1);

    for(uint32_t i = 0; i < n; i++)
    {
        uint32_t cluster = clusters[i];

        /* what do we need to read? */        
        fat_sector = (uint32_t) (info->fatsector) + ((cluster * 4) / 512);
        entry = (cluster * 4) % 512;

        /* read it! (but only if we haven't before, since PIO can be really slow) */
        error = READ(currentWorkingDrive, fat_sector, 1U, (uint8_t *) table); // TODO: TEST THIS!!!

        if(error != NULL) // error
            return; // abort

        trace("i: %i\n", i);

        if(i == (n-1)) // last cluster in the list
            table[entry] = 0x0FFFFFFF;
        else
            table[entry] = clusters[i+1] & 0x0FFFFFFF;
        trace("table: 0x%x\n", table);
        
        WRITE(currentWorkingDrive, fat_sector, 1U, (uint8_t *) table);
    }

    kfree(table);

    // this SHOULD do
}

static void save(uint32_t *clusters, uint32_t n, uint16_t *buffer, size_t buffer_size)
{

    // FIXME: could be a bug waiting to happen when using multiple partitions with multiple EBPBs
    FAT32_EBPB *bpb = (FAT32_EBPB *) info_buffer;
    const uint32_t sectclust = bpb->bpb.SectClust;
    uint32_t nlba_write = (buffer_size >> 9) /* DIV by SECTOR_SIZE */ + 1U;

    trace("first cluster: %x\t", clusters[0]);
    trace("clusters: 0x%x\t", clusters);
    trace("n: %i\t", n);
    trace("buffer: 0x%x\n", buffer);

    for(uint32_t i = 0; i < n; i++)
    {
        const uint32_t nsects = (nlba_write < sectclust) ? nlba_write : sectclust;
        uint32_t starting_lba = FAT_cluster_LBA(clusters[i]);

        // FIXME: current implementation will write garbage (or secret kernel data) to fill up cluster if the end 
        // of the buffer is smaller than exactly one cluster

        // write the file
        WRITE(currentWorkingDrive, starting_lba, (uint32_t) nsects, (uint8_t *) buffer);

        // update buffer pointer to point to the next cluster (adding a few zeroes to fill up the end of the buffer
        // would probably solve the fixme above)
        buffer += sectclust << 9;

        /* no underflows in my kingdom >:) */
        if(nlba_write >= sectclust)
            nlba_write -= sectclust;
    }
}

static uint32_t * seek_empty_clusters(uint32_t n)
{
    uint32_t *clusters = kmalloc(n * sizeof(uint32_t *));
    uint32_t c = 0;
    FS_INFO *info = FAT_getInfo();

    if(n == 0)
        return NULL;

    uint32_t i;
    for(i = (info->rootcluster); i < 0x0FFFFFF7 /* last cluster */; ++i)
    {
        // listen to whatever the fat table has to say about this cluster
        uint32_t cluster = FAT32_read_table(i);

        // ... annnnd then COMPLETELY ignore it
        // no jk, we store the cluster number if it's free 
        if( ((cluster & 0x0FFFFFFF) == 0x00))
        {
            clusters[c] = i;
            c++; // ehwww C++
        }

        // got everything we need?
        if(c >= n)
            break;
    }

    if(i >= 0x0FFFFFF8)
    {
        // seems like we don't have enough free space left!
        kfree(clusters);
        return NULL;
    }
    
    return clusters;
}

/* returns entry of the directory, or input pointer if fail */
static FAT32_DIR *FAT_find_in_dir(FAT32_DIR *dir, char *filename)
{
    uint32_t i = 0;
    char *file[12];
    FAT32_DIR *d = (FAT32_DIR *) NULL;

    FAT_convertFilenameToFATCompat(&filename[0], &file[0]);

    trace("filename: %s\n", filename);
    trace("dir fat find in dir -- 0x%x\n", dir);
        
    uint32_t entry = FAT_file_exists(dir, &file[0]);
        
    if(entry != MAX)
    {
        trace("found filename at: %x\t", entry);
        trace("filename: %s\n", &dir[entry].name[0]);
        d = &dir[entry];    
    }

    //dbg_assert(!(i >= EXIT_CODE32_FAT_FAIL));
    return d;
    
}

static uint32_t FAT_file_exists(FAT32_DIR *dir, char *filename)
{
    uint32_t i = 0;
    char file[12];

    if(strlen(filename) > FILENAME_LEN || strlen(filename) == 0)
        return MAX;

    trace("filename - file_exists: %s\n", filename);

    while(dir[i].name[0] && (i != MAX))
    {
        /* not a dir, not a file? skip! (also skip long filenames, don't care about those :) ) */
        if( ((uint8_t)dir[i].name[0]) == 0xE5 || dir[i].attrib == FAT_DIR_ATTRIB_LFN || 
            dir[i].attrib == FAT_DIR_ATTRIB_VOLUME_ID || 
            dir[i].attrib == FAT_DIR_ATTRIB_HIDDEN)
        {
            ++i;
            continue;
        }
        
        // if(strlen(dir[i].name[0]) != FILENAME_LEN)
        //     continue;

        memcpy((char *)&file[0], (char *)&(dir[i].name[0]), 8);
        memcpy((char *)&file[8], (char *)&(dir[i].ext[0]), 3);

        file[11] = '\0';
        trace("file: %s\n", &file);
        // trace("filename: %s\n", filename);
        // file exists */
        if(!strcmp((char *) &file[0], filename))
            return i;

        ++i;
    }

    // not found
    return MAX;
}

/* returns null if not found */
static FAT32_DIR *FAT_convertPath(char *path, uint32_t *dir_cluster)
{
    char *current, *backup = kmalloc(strlen(path) + 1);
    FS_INFO * info = FAT_getInfo();
    memcpy(backup, path, strlen(path));

    /* drive and partition number, ignored */
    current = strtok(backup, "/");

    uint32_t prev_cluster, 
            cluster = prev_cluster = *(dir_cluster) = (info->rootcluster);
    FAT32_DIR *dir = NULL;

    current = strtok(NULL, "/");

    // trace("current: %x\n", current);
    // trace("current: %s\n", current);
    // trace("path: %s\n\n", path);

    char file[12];

    /* search directories */
    while(1)
    {
        size_t size = 0;
        memcpy(&file[0], current, (size = strlen(current) - 1));

        // this happens often oops
        if(size > FILENAME_LEN)
            EASY_PANIC(PANIC_TYPE_DANGERZONE, "FAT_DRIVER_MEMORY_LEAK", FAT_convertPath);

        //file[size+1] = '\0';

        print("==============================================\n");

        //trace("dir: 0x%x\n", (FAT32_DIR *) buffer);
        trace("cluster: %x\n", cluster);
        trace("file: %s\n", &file[0]);
        trace("file: 0x%x\n", &file[0]);
        trace("current: %s\n", current);
        trace("current: %x\n", current);
        trace("path: %s\n\n", path);
        print("==============================================\n");

        print("HELLO FROM CONVERTPATH :)\n");

        trace("strlen(current): %i\n\n", size);

        // if this is a file, we need to make the filename compatible with
        // FAT (and not 'parent directory')
        //if(!strchr(current, '.') && strcmp(current, (char *) ".."))
        if(!strchr(path, '.') && strcmp(current, (char *) ".."))
            FAT_convertFilenameToFATCompat(current, &file[0]);
        
        uint16_t *buffer = FAT32_readDir(cluster);

        dir = FAT_find_in_dir((FAT32_DIR *) buffer, &file);

        cluster = (uint32_t) ((dir->clHi << 16U) | (dir->clLo));

        //while(1);

        // TODO: MAKE SURE EVERY FUNCTION CHECKS FOR A NULL RETURNED
        // AND IF RETURNED READS THE DIR_CLUSTER THEMSELVES
        if(dir == NULL)
        {
            // we didn't find the current thing in the directory
            kfree(buffer);
            kfree((void *) backup);
            *(dir_cluster) = prev_cluster;
            return NULL;
        }

        // save this cluster for outside-of-this-function use 
        *(dir_cluster) = cluster;

        current = strtok(NULL, "/");
        if(current == NULL)
            break;

        kfree(buffer);
        prev_cluster = cluster;
    }

    kfree((void *) backup);
    print("------------\nconvert path end\n-----------\n");
    return dir;
}

/* This function converts a filename 'file.txt' to 'file    txt'
    (aka FAT compatible)

    the argument dummy is where the new FAT compatible filename will live */
static void FAT_convertFilenameToFATCompat(char *path, char *dummy)
{
    char *current, *prev, *file = kmalloc(strlen(path) + 1);
    uint8_t spaces, i;
    char filename[12];
    size_t len;

    trace("-- dummy: 0x%x --\n", dummy);
    trace("-- &filename: 0x%x --\n", &filename[0]);
    print("---\n");

    if(strchr(path, '.'))
    {
        // seems to be fine already
        memcpy(dummy, path, FILENAME_LEN);
        return;
    }
    
    memcpy(file, path, strlen(path) + 1);
    prev = current = path;
    
    // /* since we get the entire file path, we should get rid of the path itself */
    // while(current = strtok(NULL, "/"))
    //     prev = current;

    /* we got rid of the file path and we now just have the filename!
     so let's get rid of the dot and copy the filename itself */
    current = strtok(prev, ".");
    len = (uint8_t) strlen(current);

    len = (len > FILENAME_LEN-3) ? FILENAME_LEN-3 : len;

    // trace("-- len: %x --\n", len);
    // trace("-- dummy: 0x%x --\n", dummy);
    // trace("-- dummy_val: 0x%x --\n", &dummy);
    // trace("-- &filename[0]: 0x%x --\n", &filename[0]);
    // trace("-- &current: 0x%x --\n", current);
    memcpy((char *) &filename[0], (char *) current, len);

    /* add the right amount of spaces as padding */
    spaces = (uint8_t) (8U - len);
    for(i = 0; i < spaces; ++i)
        filename[len+i] = ' ';
    trace("-- dummy: 0x%x --\n", dummy);
    /* now let's get working on the extension */
    current = strtok(NULL, ".");
    if(current != NULL)
    {
        /* if we get here the file DID have an extension, so let's copy that */
        len = ((len = strlen(current)) > 3) ? 3 : len;
        memcpy((char *) &filename[8], (char *) current, len);
    }
    else
        /* no file extension, so make sure we only put spaces there */
        len = 0;
    
    /* now add the right amount of spaces as padding */
    spaces = (uint8_t) (3U - len);
    for(i = 0; i != spaces; ++i)
        filename[8+len+i] = ' ';
        
    /* let's null terminate the string, because we can */
    filename[11] = '\0';

    //while(1);
    /* put the entie FAT compatible filename in the dummy given 
    as argument tot this function */
    print("hier anders?\n");
    memcpy( (char *) dummy, (char *) &filename[0], 12);

    kfree((void *) file);
}
