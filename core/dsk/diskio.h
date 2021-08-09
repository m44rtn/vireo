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

#ifndef __DISKIO_H__
#define __DISKIO_H__

#define SECTOR_SIZE        512
#define ATAPI_SECTOR_SIZE  2048

#define DISKIO_DISKID_HD    "HD"    // HDD
#define DISKIO_DISKID_CD    "CD"    // CD/DVD drive
#define DISKIO_DISKID_P     'P'    // partition

// defines for convert_drive_id()
#define DISKIO_DISK_NUMBER  8U
#define DISKIO_PART_NUMBER  16U

void diskio_api(void *req);

void diskio_init(void);
unsigned char *diskio_reportDrives(void);
unsigned char read(unsigned char drive, unsigned int LBA, unsigned int sctrRead, unsigned char *buf);
unsigned char write(unsigned char drive, unsigned int LBA, unsigned int sctrWrite, unsigned char *buf);

unsigned int disk_get_sector_size(unsigned char drive);

unsigned int disk_get_max_addr(unsigned char drive);
void drive_convert_to_drive_id(unsigned char drive, char *out_id);
unsigned char drive_to_type_index(unsigned char drive, unsigned char type);
const char *drive_type_to_chars(unsigned char type);

unsigned short convert_drive_id(const char *id);
unsigned char drive_type(const char *id);
unsigned char to_actual_drive(unsigned char drive, unsigned char type);

#endif