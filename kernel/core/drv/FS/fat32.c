/*
MIT license
Copyright (c) 2019-2022 Maarten Vermeulen

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

#include "fat32.h"
#include "fs_exitcode.h"

#include "../../dsk/diskdefines.h"
#include "../../include/exit_code.h"
#include "../../include/file.h"

#include "../../dsk/diskio.h"

#include "../../memory/memory.h"
#include "../../memory/paging.h"

#include "../../exec/task.h"

#include "../../hardware/driver.h"

#include "../../dsk/mbr.h"

#include "../../screen/screen_basic.h"

#include "../../dbg/dbg.h"

#include "../../util/util.h"

#include "../../kernel/panic.h"

#include "../COMMANDS.H"
#include "../FS_TYPES.H"
#include "../FS_commands.h"

#define FAT32_SECTOR_SIZE           512U /* bytes */

#define FAT_CLUSTER_TABLE_LAST_RESERVED 2

#define FAT_MAX_PARTITIONS          32

#define FAT_DIR_ATTRIB_READ_ONLY    0x01
#define FAT_DIR_ATTRIB_HIDDEN       0x02
#define FAT_DIR_ATTRIB_SYSTEM       0x04
#define FAT_DIR_ATTRIB_VOLUME_ID    0x08
#define FAT_DIR_ATTRIB_DIRECTORY    0x10
#define FAT_DIR_ATTRIB_ARCHIVE      0x20
#define FAT_DIR_ATTRIB_LFN          0x3F

#define FAT_DRIVER_FLAG_INIT_RAN    1 /* init has ran succesfully before */

#define FILE_EXT_LEN                3u
#define FILE_NAME_LEN               8u
#define TOTAL_FILENAME_LEN          (FILE_NAME_LEN + FILE_EXT_LEN)

#define FAT_CORRUPT_CLUSTER         0x0FFFFFF7

#define DIR_UNUSED_ENTRY     0xE5

typedef struct 
{
    char jmp_boot[3];
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

typedef struct
{
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

typedef struct
{
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

typedef struct fs_info_t
{
    uint8_t disk;
    uint8_t part;
   
   FAT32_EBPB *ebpb;
} __attribute__((packed)) fs_info_t;


fs_info_t partition_info[FAT_MAX_PARTITIONS];

/* the indentifier for drivers + information about our driver */
struct DRIVER FAT_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_FAT32 | DRIVER_TYPE_FS), (uint32_t) (fat_handler)};

volatile uint16_t fat_flags = 0;

void fat_handler(uint32_t *drv)
{
    drv[4] = EXIT_CODE_GLOBAL_SUCCESS;
        
    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
            if(!flag_check(fat_flags, FAT_DRIVER_FLAG_INIT_RAN))
                break;

            drv[4] = fat_init((uint8_t) drv[1], (uint8_t) drv[2]);
        break;

        case FS_COMMAND_READ:
        {
            size_t size;
            file_t *f = fat_read((const char *) drv[1], &size);

            if(f == NULL)
                drv[4] = EXIT_CODE_GLOBAL_GENERAL_FAIL;

            drv[2] = (uint32_t) f;
            drv[3] = size;
        
            break;
        }

        case FS_COMMAND_WRITE:
            drv[4] = (uint32_t) fat_write((char *) drv[1], (uint16_t *) drv[2], (size_t) drv[3], (uint8_t) drv[4]);      
        break;

        // case FS_COMMAND_RENAME:
        //     gErrorCode = FAT_setDrivePartitionActive((const char *) drv[1]);

        //     if(gErrorCode == EXIT_CODE_FS_UNSUPPORTED_DRIVE)
        //         break;

        //     FAT_rename((char *) drv[1], (char *) drv[2]);      
        // break;

        default:
            drv[4] = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
        
    }
    
}

static FAT32_EBPB *fat_get_ebpb(uint8_t disk, uint8_t part)
{
    for(uint32_t i = 0; i < FAT_MAX_PARTITIONS; ++i)
        if(partition_info[i].disk == disk && partition_info[i].part == part)
            return partition_info[i].ebpb;
    
    return NULL;
}

static uint32_t fat_cluster_lba(uint8_t disk, uint8_t part, uint32_t cluster)
{
    FAT32_EBPB* info = fat_get_ebpb(disk, part);

    uint32_t sector_of_fat = info->bpb.resvSect + MBR_getStartLBA(disk, part);
    uint32_t firstdatasect = sector_of_fat + (info->bpb.nFAT * info->sectFAT32);
    uint32_t LBA = ((cluster - 2) * (info->bpb.SectClust)) + (firstdatasect);

    return LBA;
}

static uint32_t fat_get_cluster_size(uint8_t disk, uint8_t part)
{
    FAT32_EBPB* info = fat_get_ebpb(disk, part);
    uint32_t sectclust = (info->bpb.SectClust);
    
    return sectclust * FAT32_SECTOR_SIZE;
}

static void fat_get_disk_from_path(const char *path, uint8_t *disk, uint8_t *part)
{
    char diskname[DISKIO_MAX_LEN_DISKID];
    uint32_t index = 0;

    str_get_part(&diskname[0], path, "/", &index);
    
    uint16_t driveid = drive_convert_drive_id(&diskname[0]);
    
    *disk = (uint8_t) (driveid >> DISKIO_DISK_NUMBER);
    *part = (uint8_t) (driveid >> DISKIO_PART_NUMBER);
}

static uint16_t fat_get_empty_info_entry(void)
{
    for(uint16_t i = 0; i < FAT_MAX_PARTITIONS; ++i)
        if(!partition_info[i].ebpb)
            return i;
    
    return (uint16_t) MAX;
}

err_t fat_init(uint8_t disk, uint8_t part)
{
    /* do we know this partition already? */
    if(fat_get_ebpb(disk, part))
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    
    uint32_t startLBA = MBR_getStartLBA(disk, part);
    void *buf = kmalloc(512);

    if(!buf)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;

    /* read the BPB */
    uint8_t error = read(disk, startLBA, 1U, buf);
    
    if(error)
        return error;

    uint16_t info_entry_index = fat_get_empty_info_entry();

    if(info_entry_index == (uint16_t) MAX)
        return EXIT_CODE_GLOBAL_OUT_OF_RANGE;

    fs_info_t *info_entry = &partition_info[info_entry_index];

    if(info_entry == NULL)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    
    info_entry->disk = disk;
    info_entry->part = part;
    info_entry->ebpb = buf;

    uint16_t sector_size = info_entry->ebpb->bpb.bSector;

    /* only support 512 bytes per sector */
    dbg_assert(sector_size == FAT32_SECTOR_SIZE);
    
    #ifndef NO_DEBUG_INFO
    print_value("[FAT_DRIVER] Drive: %i\n", (uint32_t) disk);
    print_value("[FAT_DRIVER] Partition: %i\n", (uint32_t) part);
    print_value("[FAT_DRIVER] Volume name: %s\n", (uint32_t) &info_entry->ebpb->volName[0]); // FIXME does not contain '\0' so keeps printing until 0 is found...
    print("\n");
    #endif

    fat_flags |= FAT_DRIVER_FLAG_INIT_RAN;

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static uint32_t fat_read_fat(uint8_t disk, uint8_t part, uint32_t cluster, uint32_t *last_sector, void *buffer)
{
    if(!buffer)
        return MAX;
    
    FAT32_EBPB *info = fat_get_ebpb(disk, part);

    if(!info)
        return MAX;
    
    /* what do we need to read? */  
    uint32_t sector_of_fat = (info->bpb.resvSect) + MBR_getStartLBA(disk, part);
    uint32_t fat_sector = (uint32_t) sector_of_fat + ((cluster * 4) / FAT32_SECTOR_SIZE);
    
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;

    // do not read from disk if we read the sector in the previous
    // 'run' (saves quite some time)
    if(*(last_sector) != fat_sector)
        err = read(disk, fat_sector, 1U, (uint8_t *) buffer);
    
    if(err)
        return NULL;

    // calculate the entry, read and return it
    uint32_t entry = (cluster * 4) % FAT32_SECTOR_SIZE;
    uint8_t *table = buffer;

    // store information for next time
    *(last_sector) = fat_sector;

    return (*((uint32_t *) &table[entry])) & 0x0FFFFFFF;
}

static void fat_write_cluster_to_table(uint8_t disk, uint8_t part, uint32_t cluster, uint32_t points_to_cluster)
{
    uint8_t *buffer = kmalloc(FAT32_SECTOR_SIZE);
    uint32_t last_sector = 0;

    fat_read_fat(disk, part, cluster, &last_sector, buffer);
    
    uint32_t entry = (cluster * 4) % FAT32_SECTOR_SIZE;

    uint32_t *table_entry = (uint32_t *) (&buffer[entry]);
    *(table_entry) = points_to_cluster & 0x0FFFFFFF;

    write(disk, last_sector, 1U, buffer);

    kfree(buffer);
}

static void fat_find_empty_cluster(uint8_t disk, uint8_t part, uint32_t *start)
{
    uint32_t last_fat_sector = 0;
    void *fat_table_buffer = kmalloc(FAT32_SECTOR_SIZE);

    uint32_t cluster = (!start) ? FAT_CLUSTER_TABLE_LAST_RESERVED + 1 : *start;
    uint32_t res = 0;

    while((res = (fat_read_fat(disk, part, cluster, &last_fat_sector, fat_table_buffer))) != 0)
        cluster++;

    kfree(fat_table_buffer);
    *start = cluster;
}

static void fat_read_cluster(uint8_t disk, uint8_t part, uint32_t cluster, void *buffer, uint32_t sect_clust)
{
    uint32_t lba = fat_cluster_lba(disk, part, cluster);
    read(disk, lba, sect_clust, buffer);
}

static uint32_t fat_search_dir_part(FAT32_DIR *dir, const char *filename, size_t size)
{
    char file[TOTAL_FILENAME_LEN + 1];
    size_t max_i = size / sizeof(FAT32_DIR);
    
    for(uint32_t i = 0; i < max_i; ++i)
    {
        if(dir[i].attrib == FAT_DIR_ATTRIB_LFN || 
            dir[i].attrib == FAT_DIR_ATTRIB_VOLUME_ID)
                continue;
        
        // copy file name and extension
        memcpy((char *)&file[0], (char *)&(dir[i].name[0]), 8);
        memcpy((char *)&file[8], (char *)&(dir[i].ext[0]), 3);

        file[TOTAL_FILENAME_LEN] = '\0';

        if(strcmp_until(&file[0], filename, FILE_NAME_LEN))
            continue;
        
        return i; 
    }

    return MAX;

}

static uint32_t fat_find_in_dir(uint8_t disk, uint8_t part, const char *filename, uint32_t starting_cluster, FAT32_DIR *odir_entry, uint32_t *dir_part_cluster)
{
    FAT32_EBPB *info = fat_get_ebpb(disk, part);
    
    if(!info)
        return MAX;

    // if no cluster is given, assume start at root dir
    uint32_t cluster = (!starting_cluster) ? info->clustLocRootdir : starting_cluster;

    uint32_t cluster_size = fat_get_cluster_size(disk, part);

    FAT32_DIR *dir_part = evalloc(cluster_size, PID_DRIVER);
    
    if(!dir_part)
        return MAX;

    // following two variables/pointers are necessary for fat_read_fat()
    uint32_t last_fat_sector = 0;
    void *fat_table_buffer = kmalloc(FAT32_SECTOR_SIZE);
    
    uint32_t index;
    
    while(cluster < FAT_CORRUPT_CLUSTER)
    {
        *dir_part_cluster = cluster;

        fat_read_cluster(disk, part, cluster, dir_part, cluster_size / FAT32_SECTOR_SIZE);
        index = fat_search_dir_part(dir_part, filename, cluster_size);

        if(index != MAX)
            break;
        
        cluster = fat_read_fat(disk, part, cluster, &last_fat_sector, fat_table_buffer);
    } 

    memcpy(odir_entry, &dir_part[index], sizeof(FAT32_DIR));

    kfree(fat_table_buffer);
    vfree(dir_part);

    return index;
} 

static void fat_filename_fatcompat(char *filename)
{
    // !! WARNING !! destroys original string

    // length of this buffer is total filename length possible for
    // FAT32 (NO LFN), plus a dot seperator and null terminator
    char original_name[TOTAL_FILENAME_LEN + 1 + 1];
    memcpy(&original_name[0], filename, TOTAL_FILENAME_LEN + 1);

    original_name[TOTAL_FILENAME_LEN + 1] = '\0';

    uint8_t dot_index = (uint8_t) (find_in_str(original_name, "."));

    dot_index = (dot_index == (uint8_t)MAX) ? FILE_NAME_LEN : dot_index;
    uint8_t n_spaces = (uint8_t) (TOTAL_FILENAME_LEN - dot_index - FILE_EXT_LEN);

    memcpy(filename, &original_name[0], dot_index);
    
    for(uint8_t i = 0; i < n_spaces; ++i)
        filename[i + dot_index] = ' ';
    
    memcpy(&filename[FILE_NAME_LEN], &original_name[dot_index + 1], FILE_EXT_LEN);
    filename[TOTAL_FILENAME_LEN] = '\0';
}

static uint32_t fat_traverse(const char *path, size_t *ofile_size, uint8_t *oattrib)
{
    uint8_t disk, part;
    fat_get_disk_from_path(path, &disk, &part);
    uint32_t start = find_in_str(path, "/");
    
    if(start == MAX)
        return 2;

    // buffer for filename is maximum characters long. At this stage
    // this is 8 chars filename, 1 seperator ('.'), 3 chars extension
    char filename[TOTAL_FILENAME_LEN + 1];
    uint32_t str_parts_index = 1;
    uint32_t starting_cluster = 0, ignore;
    FAT32_DIR dir_entry;

    while(str_get_part(&filename[0], &path[start + 1], "/", &str_parts_index))
    {
        fat_filename_fatcompat(&filename[0]);
        
        if(fat_find_in_dir(disk, part, &filename[0], starting_cluster, &dir_entry, &ignore) == MAX)
            return MAX;

        starting_cluster = (uint32_t) ((dir_entry.clHi << 16u) | dir_entry.clLo);
    }

    *ofile_size = dir_entry.fSize;
    *oattrib = dir_entry.attrib;

    return starting_cluster;
}

file_t *fat_read(const char *path, size_t *ofile_size)
{
    uint8_t disk, part;
    fat_get_disk_from_path(path, &disk, &part);
    
    uint8_t attributes;
    uint32_t starting_cluster = fat_traverse(path, ofile_size, &attributes);

    if(starting_cluster == MAX)
        return NULL;

    size_t alloc_size = *(ofile_size) + (FAT32_SECTOR_SIZE - (*(ofile_size) % FAT32_SECTOR_SIZE));
    file_t *file;
    uint8_t *buffer = file = evalloc(alloc_size, PID_DRIVER);

    if(!file)
        return NULL;
    
    // // following two variables/pointers are necessary for fat_read_fat()
    uint32_t last_fat_sector = 0;
    void *fat_table_buffer = kmalloc(FAT32_SECTOR_SIZE);
    
    uint32_t cluster = starting_cluster;
    FAT32_EBPB *info = fat_get_ebpb(disk, part);

    if(!info)
        return NULL;

    uint32_t sectclust = info->bpb.SectClust;
    size_t read_size = alloc_size;
    uint32_t i = 0;

    // read the file!
    while(cluster < FAT_CORRUPT_CLUSTER)
    {
        size_t to_read = (read_size >= (sectclust * FAT32_SECTOR_SIZE)) ? (sectclust * FAT32_SECTOR_SIZE) : read_size;
        uint32_t n_sectors = to_read / FAT32_SECTOR_SIZE + ((to_read % FAT32_SECTOR_SIZE) != 0);
        uint32_t lba = fat_cluster_lba(disk, part, cluster);

        read(disk, lba, n_sectors, &buffer[i]);       
        cluster = fat_read_fat(disk, part, cluster, &last_fat_sector, fat_table_buffer);
        
        read_size -= to_read;
        i += sectclust * FAT32_SECTOR_SIZE;
    } 

    kfree(fat_table_buffer);

    return file;
}

static void fat_get_last_from_path(char *out, char *path)
{
    // !! WARNING !! manipulates original string (char *path)

    uint32_t i, index = 0;
    while((i = find_in_str(&path[index], "/")) != MAX)
        index += (i + 1u);
    
    memcpy(out, &path[index], strlen(&path[index]));
    fat_filename_fatcompat(out);

    // place of manipulation (null terminates at the last '/' in path
    // to 'chop' the filename off)
    path[index - 1] = '\0';
}

static uint32_t fat_grow_dir(uint8_t disk, uint8_t part, uint32_t *dir_cluster)
{
    uint32_t current_cluster = *(dir_cluster);
    uint32_t cluster = 0;
    fat_find_empty_cluster(disk, part, &cluster);

    *dir_cluster = cluster;
    fat_write_cluster_to_table(disk, part, current_cluster, *dir_cluster);

    return 0;
}

static uint32_t fat_find_free_index(uint8_t disk, uint8_t part, uint32_t *dir_cluster)
{
    FAT32_EBPB *info = fat_get_ebpb(disk, part);
    
    if(!info)
        return MAX;

     // find empty entry
    char empty_entry_name[2] = {(char) (DIR_UNUSED_ENTRY), 0}; 
    FAT32_DIR dir_entry;
    uint32_t dir_part_cluster;

    uint32_t index = fat_find_in_dir(disk, part, &empty_entry_name[0], *dir_cluster, &dir_entry, &dir_part_cluster);

    // if no empty entry then get the last entry of the directory
    if(index == MAX)
    {
        empty_entry_name[0] = 0;
        index = fat_find_in_dir(disk, part, &empty_entry_name[0], *dir_cluster, &dir_entry, &dir_part_cluster);
    }

    *dir_cluster = dir_part_cluster;

    uint32_t cluster_size = fat_get_cluster_size(disk, part);

    // check if the entry is the very last one that can fit on a cluster
    // grow directory if so.
    if(((index * sizeof(FAT32_DIR)) + sizeof(FAT32_DIR) >= cluster_size) &&
        empty_entry_name[0] == (char)0)
            index = fat_grow_dir(disk, part, dir_cluster);
    
    return index;
}

static err_t fat_write_dir(uint8_t disk, uint8_t part, uint32_t fcluster, uint32_t dir_cluster, size_t fsize, uint8_t attrib, char *filename)
{
    uint32_t current_dir_cluster = dir_cluster;
    uint32_t index = fat_find_free_index(disk, part, &dir_cluster);

    if(index == MAX)
        return EXIT_CODE_FS_NO_SPACE;

    uint32_t cluster_size = fat_get_cluster_size(disk, part);

    void *b = evalloc(cluster_size, PID_DRIVER);
    FAT32_DIR *dir_part = b;

    uint32_t sectclust = cluster_size / FAT32_SECTOR_SIZE;
    read(disk, fat_cluster_lba(disk, part, dir_cluster), sectclust, (uint8_t *) dir_part);

    dir_part[index].attrib = attrib;
    dir_part[index].fSize = fsize;
    dir_part[index].clHi = (uint16_t) (fcluster >> 16u);
    dir_part[index].clLo = (uint16_t) (fcluster & 0xFFFF);

    memcpy(&dir_part[index].name[0], filename, TOTAL_FILENAME_LEN);

    if(current_dir_cluster != dir_cluster)
        dir_part[index + 1].name[0] = (char) DIR_UNUSED_ENTRY;
    
    write(disk, fat_cluster_lba(disk, part, dir_cluster), sectclust, (uint8_t *) b);

    vfree(b);
    return EXIT_CODE_GLOBAL_SUCCESS;
}

static err_t fat_write_new(uint8_t disk, uint8_t part, char *filename, uint32_t dir_cluster, file_t *buffer, size_t filesize, uint8_t attrib)
{
    FAT32_EBPB *info = fat_get_ebpb(disk, part);
    
    if(!info)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    uint32_t cluster_size = fat_get_cluster_size(disk, part);

    uint32_t cluster = 0;
    fat_find_empty_cluster(disk, part, &cluster);

    // read the directory cluster (only the cluster we need to change anything)
    // and update information
    err_t err = fat_write_dir(disk, part, cluster, dir_cluster, filesize, attrib, filename);

    if(err)
        return err;
    
    // write the directory to the disk    
    uint32_t sectclust = cluster_size / FAT32_SECTOR_SIZE;
    uint8_t *temp_buffer = evalloc(cluster_size, PID_DRIVER);
    uint32_t i = 0;
    
    while(cluster < FAT_CORRUPT_CLUSTER)
    {
        size_t size = (filesize >= (sectclust * FAT32_SECTOR_SIZE)) ? (sectclust * FAT32_SECTOR_SIZE) : filesize;
        
        memcpy(temp_buffer, (void *) ((uint32_t)*(&buffer) + i), size);
        memset(&temp_buffer[size], (sectclust * FAT32_SECTOR_SIZE) - size, 0);

        uint32_t lba = fat_cluster_lba(disk, part, cluster);
        write(disk, lba, sectclust,  temp_buffer);

        filesize -= size;
        
        uint32_t next_cluster = cluster + 1;
        
        if(filesize)
            fat_find_empty_cluster(disk, part, &next_cluster);
        else next_cluster = MAX & 0x0FFFFFFF;

        fat_write_cluster_to_table(disk, part, cluster, next_cluster);
        cluster = next_cluster;    
        i += size;
    }

    vfree(temp_buffer);
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

err_t fat_write(const char *path, file_t *buffer, size_t file_size, uint8_t attrib)
{
    char *working_path = create_backup_str(path);

    uint8_t disk, part;
    fat_get_disk_from_path(working_path, &disk, &part);

    if(!working_path)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
        
    char filename[TOTAL_FILENAME_LEN + 1];
    
    // remove last part of the working path (file to store)
    // and move it to the filename buffer
    fat_get_last_from_path(&filename[0], working_path);

    // get the cluster of the directory to save the file in
    size_t dir_size;
    uint8_t dir_attrib;
    uint32_t dir_cluster = fat_traverse(working_path, &dir_size, &dir_attrib);

    if(dir_cluster == MAX)
        return EXIT_CODE_FS_FILE_NOT_FOUND;
    
    // check if file already exists
    FAT32_DIR dir_entry;
    uint32_t dir_part_cluster;
    uint32_t dir_index = fat_find_in_dir(disk, part, &filename[0], dir_cluster, &dir_entry, &dir_part_cluster);

    // if the file was found and it is read-only then do nothing
    if(dir_index != MAX && (dir_entry.attrib & FAT_DIR_ATTRIB_READ_ONLY))
        return EXIT_CODE_FS_FILE_READ_ONLY;
    
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    if(dir_index == MAX)
        err = fat_write_new(disk, part, &filename[0], dir_cluster, buffer, file_size, attrib);
    // TODO:
    //else
    //    err = fat_write_existing(disk, part, dir_part_cluster, &dir_entry, dir_index, buffer, file_size, attrib);

    kfree(working_path);
    return err;
}
