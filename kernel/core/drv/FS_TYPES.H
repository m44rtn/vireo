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

#ifndef __FS_TYPES_H__
#define __FS_TYPES_H__

#include "../include/types.h"

#define FAT_MAX_FILENAME_LEN        11
#define ISO_MAX_FILENAME_LEN	    255 // bytes

#define FAT_FILE_ATTRIB_FILE        0x00
#define FAT_FILE_ATTRIB_READONLY    0x01
#define FAT_FILE_ATTRIB_HIDDEN      0x02
#define FAT_FILE_ATTRIB_SYSTEM      0x04
#define FAT_FILE_ATTRIB_DIR         0x10

#define FS_TYPE_FAT12   0x01
#define FS_TYPE_FAT16   0x04
#define FS_TYPE_FAT32   0x0B

#define FS_TYPE_ISO     0x96

// FAT time and date offsets
#define FAT_SECOND_OFFSET		0
#define FAT_MINUTE_OFFSET		5
#define FAT_HOUR_OFFSET			11

#define FAT_DAY_OFFSET		    0
#define FAT_MONTH_OFFSET		5
#define FAT_YEAR_OFFSET			9

typedef struct fs_file_info_t
{
    uint32_t first_cluster; // 0xFFFFFFFF if not applicable
    uint32_t first_sector;
    size_t file_size;
    uint8_t file_type;      // in FAT32 format (file attributes)
    uint32_t creation_date; // in FAT32 format
    uint32_t creation_time; // in FAT32 format
    uint32_t access_date;   // in FAT32 format
    uint32_t modified_date; // in FAT32 format
    uint32_t modified_time; // in FAT32 format
} __attribute__((packed)) fs_file_info_t;


typedef struct fs_dir_contents_t
{
    char name[ISO_MAX_FILENAME_LEN + 1];
    uint8_t attrib;         // in FAT32 format
    size_t file_size;
} __attribute__((packed)) fs_dir_contents_t;


#endif