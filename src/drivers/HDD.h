#ifndef HDD_H
#define HDD_H

#include "../include/types.h"
#include "ATA/ATA.h"
#include "FS/FAT.h"
#include "../include/DEFATA.h"
#include "../include/VFS.h"

void get_drive_info();

uint32_t GetFirstSectLBA(uint8_t drive, uint8_t DriveType);

#endif