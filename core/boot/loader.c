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

#include "loader.h"

#include "multiboot.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../util/util.h"

#ifndef NO_DEBUG_INFO
#include "../screen/screen_basic.h"
#endif

#define LOADER_MAGICNUMBER_MULTIBOOT    0x2BADB002

static void loader_multiboot_compliant(void);
static void loader_multiboot_convertInfoStruct(void);
 
extern const uint32_t MAGICNUMBER;
extern uint32_t *BOOTLOADER_STRUCT_ADDR;

static uint8_t loader_type = LOADER_TYPE_UNKNOWN;

LOADER_INFO loader_info;

uint8_t loader_detect(void)
{
    memset(&loader_info, sizeof(LOADER_INFO), 0);

    if(MAGICNUMBER == LOADER_MAGICNUMBER_MULTIBOOT)
        loader_multiboot_compliant();  
    else 
        return EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;

    return EXIT_CODE_GLOBAL_SUCCESS; 
}

uint8_t loader_get_type(void)
{
    return loader_type;
}

uint32_t loader_get_boot_drive(void)
{
    return loader_info.boot_drive;
}

LOADER_INFO loader_get_infoStruct(void)
{
    return loader_info;
}

uint32_t *loader_get_multiboot_info_location(void)
{
    return BOOTLOADER_STRUCT_ADDR;
}

static void loader_multiboot_compliant(void)
{
#ifndef NO_DEBUG_INFO
    multiboot_info_t *info = (multiboot_info_t *) BOOTLOADER_STRUCT_ADDR;

    char *bootloader_name = (char *) info->boot_loader_name;

    print( "[LOADER] Reports multiboot compliant\n");
    print_value( "[LOADER] Loaded by %s\n\n", (unsigned int) bootloader_name);
#endif

    loader_type   = LOADER_TYPE_MULTIBOOT;
    loader_multiboot_convertInfoStruct();
}

static void loader_multiboot_convertInfoStruct(void)
{
    multiboot_info_t *info = (multiboot_info_t *) BOOTLOADER_STRUCT_ADDR;

    if(info->flags & 0x01)
        loader_info.boot_drive = info->boot_device;

    if(info->flags & 0x40)
    {
        loader_info.mmap = (uint32_t *) info->mmap_addr;    
        loader_info.mmap_length = info->mmap_length;
    } 
    
    loader_info.total_memory = info->mem_upper + info->mem_lower; 
}


