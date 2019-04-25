#ifndef VFS_H
#define VFS_H

#include "types.h"

#define VFS_DRIVE_HD0 0x01
#define VFS_DRIVE_HD1 0x10

#define VFS_DRIVE_CD0 0x02
#define VFS_DRIVE_CD1 0x20

typedef struct{
    //uint8_t drive;
    uint32_t cluster;
} tVFS;

typedef struct{
    uint8_t HD0; //drive ATA number for HD0 (location of BIRDOS folder)
} tVFS_INFO;

tVFS_INFO vfs_info;


#endif