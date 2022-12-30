/*
MIT license
Copyright (c) 2021 Maarten Vermeulen

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

#ifndef __DISK_H__
#define __DISK_H__

#include "types.h"
#include "call.h"

#define DISK_ID_MAX_SIZE        8 // bytes
#define DISK_SECTOR_SIZE        512 //bytes

typedef struct disk_info_t
{
    char name[DISK_ID_MAX_SIZE];
    size_t sector_size;
    size_t disk_size;
} __attribute__((packed)) disk_info_t;

typedef struct partition_info_t
{
    uint32_t starting_sector;
    uint32_t n_sectors;
    uint8_t type;
} __attribute__((packed)) partition_info_t;

// returns information on detected disks by the system and the total size of the list in *size
disk_info_t *disk_get_drive_list(size_t *size);

// returns information about a partition of a disk (e.g. HD0P0)
partition_info_t *disk_get_partition_info(char *_id);

// returns the buffer of SECTOR_SIZE * _sctrs read at _lba (or NULL if fail)
void *disk_absolute_read(char *_drive, uint32_t _lba, uint32_t _sctrs);

// writes (_bfr_size / SECTOR_SIZE + (_bfr_size % SECTOR_SIZE != 0)) sectors from _bfr, starting at _lba 
err_t disk_absolute_write(char *_drive, uint32_t _lba, void *_bfr, size_t _bfr_size);

// returns the bootdisk (e.g., HD0P0 or CD0)
char *disk_get_bootdisk(void);

#endif // __DISK_H__