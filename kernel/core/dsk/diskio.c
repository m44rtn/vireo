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

void diskio_api(void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *) req;

    switch(hdr->system_call)
    {
        case SYSCALL_DISK_LIST: // GET DRIVE LIST
        {
            // TODO make function
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
            // TODO make function
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
            // TODO: make function
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
            // TODO: make function
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
            
            hdr->exit_code = EXIT_CODE_GLOBAL_SUCCESS;
        break;

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}

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

uint8_t *diskio_reportDrives(void)
{
    uint32_t i = 0;
    uint8_t *drive_list = (uint8_t *) kmalloc(DISKIO_MAX_DRIVES*sizeof(uint32_t));

    for(; i < IDE_DRIVER_MAX_DRIVES; ++i)
        drive_list[i] = disk_info_t[i].disktype;

    return drive_list;
}

uint8_t read(unsigned char drive, unsigned int LBA, unsigned int sctrRead, unsigned char *buf)
{
    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint8_t disk_type = disk_info_t[drive].disktype;
    uint32_t command = (disk_type == DRIVE_TYPE_IDE_PATA || disk_type == DRIVE_TYPE_IDE_PATAPI ) ?
            IDE_COMMAND_READ : NULL; /* TODO: make NULL floppy command */

    if(drive > DISKIO_MAX_DRIVES)
        return EXIT_CODE_GLOBAL_OUT_OF_RANGE;

    drv[0] = command;
    drv[1] = (uint32_t) (drive);
    drv[2] = LBA;
    drv[3] = sctrRead;
    drv[4] = (uint32_t) (buf);

    driver_exec_int((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);

    kfree(drv);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

uint8_t write(unsigned char drive, unsigned int LBA, unsigned int sctrWrite, unsigned char *buf)
{
    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint32_t command = (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATA) ?
            IDE_COMMAND_WRITE : NULL; /* TODO: make NULL floppy command */

    drv[0] = command;
    drv[1] = (uint32_t) (drive);
    drv[2] = LBA;
    drv[3] = sctrWrite;
    drv[4] = (uint32_t) (buf);

    driver_exec_int((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);

    kfree(drv);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

size_t disk_get_max_addr(uint8_t drive)
{

    uint32_t *drv = kmalloc(sizeof(uint32_t) * DRIVER_COMMAND_PACKET_LEN);
    uint32_t command = (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATA) ?
            IDE_COMMAND_GET_MAX_ADDRESS : NULL; /* TODO: make NULL floppy command */

    drv[0] = command;
    drv[1] = (uint32_t) (drive);

    driver_exec_int((uint32_t) (disk_info_t[drive].controller_info | DRIVER_TYPE_PCI), drv);
    
    return drv[2];
}

size_t disk_get_sector_size(uint8_t drive)
{
    return (disk_info_t[drive].disktype == DRIVE_TYPE_IDE_PATAPI) ? ATAPI_DEFAULT_SECTOR_SIZE : DEFAULT_SECTOR_SIZE;
}

void drive_convert_to_drive_id(uint8_t drive, char *out_id)
{
    uint8_t type = disk_info_t[drive].disktype;

    char *t = (char *) (drive_type_to_chars(type));
    size_t s = strlen(t);

    memcpy(out_id, t, s);
    out_id[s] = (char) (drive_to_type_index(drive, type) + '0');
}

// @returns:
//   - most significant byte: actual drive number
//   - least significant byte: actual partition number (when applicable)
uint16_t drive_convert_drive_id(const char *id)
{
    uint8_t drive, type;
    uint16_t result = 0;

    // get drive type
    type = drive_type(id);

    if(type == ((uint8_t) MAX))
        return type;

    // get drive number (drive id of that type)
    drive = strdigit_toInt(id[2]);
    
    if(drive == EXIT_CODE_GLOBAL_UNSUPPORTED)
        return (uint16_t) MAX;

    // convert drive id of type to the actual real world drive number
    drive = to_actual_drive(drive, type);

    // store it
    result = (uint16_t) (result | (drive & 0xFFU) << DISKIO_DISK_NUMBER);

    // is there a partition specified?
    if(id[3] != DISKIO_DISKID_P)
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

uint8_t drive_type(const char *id)
{
    uint8_t type;
    
    // check drive type
    if(!strcmp_until(&id[0], DISKIO_DISKID_HD, 2)) // is hdd?
        type = DRIVE_TYPE_IDE_PATA;
    else if(!strcmp_until(&id[0], DISKIO_DISKID_CD, 2)) // is cd?
        type = DRIVE_TYPE_IDE_PATAPI;
    else
        return (uint8_t) MAX;
    
    return type;
}

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
                return i;
        }
    }

    kfree(drivelist);
    return (uint8_t) MAX;
}

// function converts a drive number to the 'th drive of that type
// (i.e. the opposite of to_actual_drive())
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

const char *drive_type_to_chars(uint8_t type)
{
    if(type == DRIVE_TYPE_IDE_PATA)
        return (const char *) DISKIO_DISKID_HD;
    else if (type == DRIVE_TYPE_IDE_PATAPI)
        return (const char *) DISKIO_DISKID_CD;
    
    return (const char *) " ";
}
