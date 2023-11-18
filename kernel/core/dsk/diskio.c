/*
MIT license
Copyright (c) 2019-2023 Maarten Vermeulen

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

#include "mbr.h"
#include "bootdisk.h"

#include "../include/types.h"
#include "../dsk/diskdefines.h"
#include "../include/exit_code.h"

#include "../hardware/driver.h"
#include "../hardware/pci.h"

#include "../memory/memory.h"
#include "../memory/paging.h"

#include "../util/util.h"

#include "../drv/IDE_commands.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#define DISK_ID_MAX_SIZE        8

// api stuff
typedef struct disk_info_t
{
    char name[DISK_ID_MAX_SIZE];
    size_t sector_size;
    size_t disk_size;
} __attribute__((packed)) api_disk_info_t;

typedef struct partition_info_t
{
    uint32_t starting_sector;
    uint32_t n_sectors;
    uint8_t type;
} __attribute__((packed)) api_partition_info_t;

typedef struct disk_syscall_t
{
    syscall_hdr_t hdr;
    char *drive;
    uint32_t lba;
    uint32_t nlba;
    size_t buffer_size;
    void *buffer;
} __attribute__((packed)) disk_syscall_t;
// -- end api stuff

typedef struct{
    uint8_t diskID;
    uint8_t disktype;
    uint16_t controller_info;
}__attribute__((packed)) DISKINFO;

DISKINFO disk_info_t[DISKIO_MAX_DRIVES];

/**
 * @brief API handler for disk I/O such as drive lists, absolute disk writes/reads and partition info
 * 
 * @param req Pointer to API request
 */
void diskio_api(void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *) req;

    switch(hdr->system_call)
    {
        case SYSCALL_DISK_LIST: // GET DRIVE LIST
        {
            uint8_t *disks = diskio_reportDrives();
            api_disk_info_t *dsk = (api_disk_info_t *) evalloc(DISKIO_MAX_DRIVES * sizeof(api_disk_info_t), prog_get_current_running());
            
            for(uint8_t i = 0; i < DISKIO_MAX_DRIVES; ++i)
            {
                if(disks[i] == DRIVE_TYPE_UNKNOWN)
                    break;

                dsk[i].disk_size = disk_get_max_addr(i) * disk_get_sector_size(i);

                const char *id = drive_type_to_chars(disks[i]);
                memcpy(&dsk[i].name[0], (char *) (id), strlen(id));

                dsk[i].sector_size = disk_get_sector_size(i);
            }

            hdr->response_ptr = (void *) dsk;
            hdr->response_size = DISKIO_MAX_DRIVES * sizeof(api_disk_info_t);

            kfree(disks);
            break;
        }

        case SYSCALL_PARTITION_INFO: // GET PARTITION INFO
        {
            disk_syscall_t *c = (disk_syscall_t *) req;

            uint16_t id = drive_convert_drive_id(c->drive);

            uint8_t disk = (uint8_t) ((id >> 8) & 0xFFU);
            uint8_t part = (uint8_t) id & 0xFFU;

            api_partition_info_t *p = (api_partition_info_t *) evalloc(sizeof(api_partition_info_t), prog_get_current_running());

            p->n_sectors = mbr_get_sector_count(disk, part);
            p->starting_sector = MBR_getStartLBA(disk, part);
            p->type = mbr_get_type(disk, part);

            c->hdr.response_ptr = (void *) p;
            c->hdr.response_size = sizeof(api_partition_info_t);

            break;
        }

        case SYSCALL_DISK_ABS_READ:
        {
            disk_syscall_t *c = (disk_syscall_t *) req;

            uint16_t id = drive_convert_drive_id((const char *) c->drive);
            uint8_t drive =  (uint8_t) ((id >> 8) & 0xFF);
            uint8_t part = (uint8_t) (id & 0xFF);

            uint8_t *b = evalloc(c->nlba * DEFAULT_SECTOR_SIZE, prog_get_current_running());
            
            if(!b)
                { c->hdr.exit_code = EXIT_CODE_GLOBAL_OUT_OF_MEMORY; break; }
            
            uint32_t lba = c->lba;

            if((drive_type(c->drive) == DRIVE_TYPE_IDE_PATAPI) && part != 0xFF)
                lba = c->lba + MBR_getStartLBA(drive, part);

            c->hdr.exit_code = read(drive, lba, c->nlba, b);

            c->hdr.response_ptr = b;
            c->hdr.response_size = c->nlba * DEFAULT_SECTOR_SIZE;
            
            break;
        }

        case SYSCALL_DISK_ABS_WRITE:
        {
            disk_syscall_t *c = (disk_syscall_t *) req;
            
            if(c->buffer_size == 0)
                { c->hdr.exit_code = EXIT_CODE_GLOBAL_OUT_OF_RANGE; break; }
            if(c->buffer < (void *) memory_get_malloc_end() /* (end of kernel space) */) 
                { c->hdr.exit_code = EXIT_CODE_GLOBAL_RESERVED; break; }
            if(drive_type(c->drive) == DRIVE_TYPE_IDE_PATAPI)
                { c->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break; }

            uint16_t id = drive_convert_drive_id((const char *) c->drive);
            uint8_t drive =  (uint8_t) ((id >> 8) & 0xFF);
            uint8_t part = (uint8_t) (id & 0xFF);

            uint32_t lba = c->lba;

            if(part != 0xFF)
                lba = lba +  + MBR_getStartLBA(drive, part);
            
            uint32_t nlba = (c->buffer_size / DEFAULT_SECTOR_SIZE) + ((c->buffer_size % DEFAULT_SECTOR_SIZE) != 0);

            c->hdr.exit_code = write(drive, lba, nlba, (uint8_t *) c->buffer);
            break;
        }

        case SYSCALL_DISK_GET_BOOTDISK:
            hdr->response_ptr = evalloc(DISK_ID_MAX_SIZE, prog_get_current_running());

            char *id = bootdisk();
            memcpy(hdr->response_ptr, id, strlen(id));
            kfree(id);
            
            hdr->exit_code = EXIT_CODE_GLOBAL_SUCCESS;
        break;

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}

/**
 * @brief Initializes disk I/O functions and caches the information about connected disks
 * 
 */
void diskio_init(void)
{
    uint8_t i;
    uint32_t *devicelist, IDE_ctrl;
    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN + sizeof(uint32_t) * MAX_DRIVES);
    
    // drive list
    uint8_t *drives = (uint8_t *)((uint32_t)drv) + sizeof(uint32_t)*DRIVER_COMMAND_PACKET_LEN;

    devicelist = pciGetDevices(0x01, 0x01);
    IDE_ctrl = devicelist[1];
    kfree(devicelist);

    // prepare and exec IDE report command
    drv[0] = IDE_COMMAND_REPORTDRIVES;
    drv[1] = (uint32_t) (drives);
    driver_exec_int(pciGetInfo(IDE_ctrl) | DRIVER_TYPE_PCI, drv); 

    for(i = 0; i < DISKIO_MAX_DRIVES; ++i)
    {
        // disks are returned in order with their type being stored at the 
        // index of the drive number (see IDE_commands.h for more info)
        disk_info_t[i].disktype = (uint8_t) drives[i];
        disk_info_t[i].diskID = i; 
        disk_info_t[i].controller_info = (uint16_t) pciGetInfo(IDE_ctrl);
    }

    kfree(drv);
}

/**
 * @brief Checks whether a drive or partition exists
 * 
 * @param id drive id in human readable format (e.g., 'CD0' or 'HD0P0')
 *           NOTE: this function is able to ignore whatever is behind the drive id
 * @return true This drive/partition exists
 * @return false This drive/partition does not exist
 */
bool diskio_check_exists(const char *id)
{
    uint16_t disk_part = drive_convert_drive_id(id);

    uint8_t disk = (uint8_t) (disk_part >> 8);
    uint8_t part = (uint8_t) (disk_part);

    if(disk == 0xFF || disk >= MAX_DRIVES)
        return false;

    if(disk_info_t[disk].disktype == 0xFF)
        return false;
    
    if(disk_info_t[disk].disktype == DRIVE_TYPE_IDE_PATAPI)
        return true;
    
    // if we get here it was a hard drive
    if(mbr_get_type(disk, part) != 0xFF)
        return true;
    
    return false;
}

/**
 * @brief Returns a list of drives attached to the system.
 *        NOTE: Allocates kernel memory (caller needs to free result)
 * 
 * @return uint8_t* Pointer to list of attached drives.
 *                  Format: list[drive_id == index] = disk_type (e.g., PATA or PATAPI)
 */
uint8_t *diskio_reportDrives(void)
{
    uint32_t i = 0;
    uint8_t *drive_list = (uint8_t *) kmalloc(DISKIO_MAX_DRIVES*sizeof(uint32_t));

    for(; i < IDE_DRIVER_MAX_DRIVES; ++i)
        drive_list[i] = disk_info_t[i].disktype;

    return drive_list;
}

/**
 * @brief Absolute read at LBA on drive, uses internal drivers
 * 
 * @param drive drive number
 * @param LBA sector number
 * @param sctrRead amount of sectors to read
 * @param buf output buffer for content
 * @return uint8_t exit code (any error by driver or EXIT_CODE_GLOBAL_OUT_OF_RANGE if drive number is too large)
 */
uint8_t read(unsigned char drive, unsigned int LBA, unsigned int sctrRead, unsigned char *buf)
{
    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint8_t disk_type = disk_info_t[drive].disktype;
    uint32_t command = (disk_type == DRIVE_TYPE_IDE_PATA || disk_type == DRIVE_TYPE_IDE_PATAPI ) ?
            IDE_COMMAND_READ : NULL; // NULL is here for support for another driver if it's added

    if(drive > DISKIO_MAX_DRIVES)
        { kfree(drv); return EXIT_CODE_GLOBAL_OUT_OF_RANGE; }

    drv[0] = command;
    drv[1] = (uint32_t) (drive);
    drv[2] = LBA;
    drv[3] = sctrRead;
    drv[4] = (uint32_t) (buf);

    driver_exec_int((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);

    kfree(drv);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

/**
 * @brief Absolute write at LBA on drive, uses internal drivers
 * 
 * @param drive drive number
 * @param LBA sector number
 * @param sctrRead amount of sectors to write
 * @param buf output buffer for content
 * @return uint8_t exit code (any error by driver)
 */
uint8_t write(unsigned char drive, unsigned int LBA, unsigned int sctrWrite, unsigned char *buf)
{
    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint32_t command = (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATA) ?
            IDE_COMMAND_WRITE : NULL; 

    drv[0] = command;
    drv[1] = (uint32_t) (drive);
    drv[2] = LBA;
    drv[3] = sctrWrite;
    drv[4] = (uint32_t) (buf);

    driver_exec_int((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);

    kfree(drv);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

/**
 * @brief Gets the maximum address of a drive (i.e., drive size)
 * 
 * @param drive drive number
 * @return size_t drive size, in amount of sectors
 */
size_t disk_get_max_addr(uint8_t drive)
{

    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint32_t command = (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATA) ?
            IDE_COMMAND_GET_MAX_ADDRESS : NULL;

    drv[0] = command;
    drv[1] = (uint32_t) (drive);

    driver_exec_int((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);
    
    size_t max_addr = drv[2];
    kfree(drv);

    return max_addr;
}

/**
 * @brief Returns the size of a sector based on the drive type
 * 
 * @param drive drive number
 * @return size_t size of sector in bytes
 */
size_t disk_get_sector_size(uint8_t drive)
{
    return (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATAPI) ? ATAPI_DEFAULT_SECTOR_SIZE : DEFAULT_SECTOR_SIZE;
}

/**
 * @brief Converts a numbered drive id to a human readable drive id (e.g., 'CD0' or 'HD0P0')
 *        which it returns in `out_id`
 * 
 * @param drive drive number
 * @param out_id output: drive id
 */
void drive_convert_to_drive_id(uint8_t drive, char *out_id)
{
    uint8_t type = disk_info_t[drive].disktype;

    char *type_str = (char *) (drive_type_to_chars(type));
    size_t s = strlen(type_str);

    memcpy(out_id, type_str, s);
    out_id[s] = (char) (drive_to_type_index(drive, type) + '0');
    out_id[s+1] = '\0';
}

/**
 * @brief Converts human readable drive id (e.g., 'CD0' or 'HD0P0')
 *        to a more machine oriented drive id.
 * 
 * @param id Human readable drive id (e.g., 'CD0' or 'HD0P0')
 * @return uint16_t [drive number][partition number] (most significant byte, least signifcant byte respectively)
 *                  NOTE: [partition number] == 0xFF if not applicable/supported
 */
uint16_t drive_convert_drive_id(const char *id)
{
    uint8_t drive, type;
    uint16_t result = 0;
    
    // get drive type
    type = drive_type(id);

    if(type == ((uint8_t) MAX))
        return (uint16_t) MAX;

    // get drive number (drive id of that type)
    drive = strdigit_toInt(id[2]);
    
    if(drive == EXIT_CODE_GLOBAL_UNSUPPORTED)
        return (uint16_t) MAX;

    // convert drive id of type to the actual real world drive number
    drive = to_actual_drive(drive, type);

    // store it
    result = (uint16_t) (result | (drive & 0xFFU) << DISKIO_DISK_NUMBER);

    // is there a partition specified?
    if(id[3] != DISKIO_DISKID_P && id[3] != (DISKIO_DISKID_P + 0x20))
        return result | 0xFF;

    // if yes
    // get its number
    drive = strdigit_toInt(id[4]);
    
    if(drive == EXIT_CODE_GLOBAL_UNSUPPORTED)
        return (uint16_t) MAX;
    
    // store it (drive = partition number)
    result = (uint16_t) (result | (drive & 0xFF));

    return result;
}

/**
 * @brief Converts human readble drive type (e.g., 'CD' or 'HD') to the
 *          internal kernel equivilant drive type.
 * 
 * @param _id human readble drive type (e.g., 'CD' or 'HD')
 * @return uint8_t drive type, or MAX if unsupported
 */
uint8_t drive_type(const char *_id)
{
    uint8_t type;
    char id[4];

    memcpy(id, _id, 4);
    to_uc(id, 4);
    
    // check drive type
    if(!strcmp_until(&id[0], DISKIO_DISKID_HD, 2)) // is hdd?
        type = DRIVE_TYPE_IDE_PATA;
    else if(!strcmp_until(&id[0], DISKIO_DISKID_CD, 2)) // is cd?
        type = DRIVE_TYPE_IDE_PATAPI;
    else
        return (uint8_t) MAX;
    
    return type;
}

/**
 * @brief Uses the human readable drive id (e.g. '0' in 'HD0')
 *          and drive type to get the kernel's internal drive id for this disk
 * 
 * @param drive drive number of human readable format (e.g. '0' in 'HD0'), as uint8_t
 * @param type drive type id
 * @return uint8_t drive number
 */
uint8_t to_actual_drive(uint8_t drive, uint8_t type)
{
    uint8_t *drivelist = diskio_reportDrives();
    uint8_t nfound = 0;

    // if we get drive number 0 as argument, we need the first drive
    // we find, thus it should be at least one (the offset is also 1)
    drive++;
    
    for(uint8_t i = 0; i < DISKIO_MAX_DRIVES; ++i)
    {
        if(drivelist[i] == type)
        {
            nfound++;

            if(nfound == drive)
                { kfree(drivelist); return i; }
        }
    }

    kfree(drivelist);
    return (uint8_t) MAX;
}

/**
 * @brief Uses drive number and type number to convert to the drive id number for
 *          human readable format (e.g., '0' in 'HD0')
 *          (i.e. the opposite of to_actual_drive())
 * 
 * @param drive drive number
 * @param type drive type
 * @return uint8_t nth drive of this type: drive id number for
 *          human readable format (e.g., '0' in 'HD0')
 */
uint8_t drive_to_type_index(uint8_t drive, uint8_t type)
{
    uint8_t *drivelist = diskio_reportDrives();
    uint8_t nfound = 0;

    for(uint8_t i = 0; i < drive; ++i)
    {
        if(drivelist[i] == type)
            nfound++;
    }

    kfree(drivelist);
    return (uint8_t) nfound;
}

/**
 * @brief Converts drive type number to character format (e.g., 'HD' or 'CD')
 * 
 * @param type type number
 * @return const char* character format (human readable format): e.g., 'HD' or 'CD'
 *                      or one space (" ") if not supported
 */
const char *drive_type_to_chars(uint8_t type)
{
    if(type == DRIVE_TYPE_IDE_PATA)
        return (const char *) DISKIO_DISKID_HD;
    else if (type == DRIVE_TYPE_IDE_PATAPI)
        return (const char *) DISKIO_DISKID_CD;
    
    return (const char *) " ";
}
