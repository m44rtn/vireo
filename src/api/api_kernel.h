#ifndef API_KERNEL_H
#define API_KERNEL_H

#include "../include/types.h"
#include "../include/info.h"
#include "../include/error.h"
#include "../io/memory.h"
#include "../io/multitasking.h"

void api_init(uint32_t *api_struct);

//#define KMEMSET_KOTHER 0xB17D05 //BirdOS

#endif