#ifndef V86_H
#define V86_H

#include "../io/multitasking.h"
#include "DriverHandler.h"
#include "../io/memory.h"

void setup_v86(uint32_t file_start, uint32_t file_size, uint16_t flags);


#endif