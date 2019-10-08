/*
MIT license
Copyright (c) 2019 Maarten Vermeulen

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

#include "memory.h"

#include "../include/types.h"
#include "../include/global_exit_codes.h"

#include "../boot/loader.h"
#include "../boot/multiboot.h"

#include "../screen/screen_basic.h"

typedef struct
{
    uint32_t available_memory; /* in bytes */
    uint32_t *usable_memory;   /* is an array */
} MEMORY_INFO;

uint8_t loader_type = 0;

static void memory_get_multiboot(multiboot_info_t *mbt);

uint8_t memory_init()
{
    uint32_t *infoStruct;

    loader_type = loader_get_type();

    /* If the loader type is unknown, we won't get an info struct.
       TODO: implement int 15h ax=0xe820
       FIXME: remove general fail exit code after implementing int 15h */
    if(type == LOADER_TYPE_UNKNOWN)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    infoStruct = loader_get_infoStruct();

    if(infoStruct == NULL)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    if(type == LOADER_TYPE_MULTIBOOT)
        memory_get_multiboot((multiboot_info_t *) infoStruct);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static void memory_get_multiboot(multiboot_info_t *mbt)
{

}