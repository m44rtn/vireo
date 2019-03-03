#ifndef HDD_H
#define HDD_H

#include "../include/types.h"
#include "ATA/ATA.h"
#include "../include/DEFATA.h"

uint32_t GetFirstSectLBA(uint8_t drive, uint8_t DriveType);

#endif