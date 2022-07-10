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

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "types.h"
#include "call.h"
#include "fs.h"

#define DRIVER_MAX_NAME_SIZE    32

#define DRIVER_TYPE_PCI             0x01 << 24
#define DRIVER_TYPE_FS              0x02 << 24
#define DRIVER_TYPE_HID             0x03 << 24 // Human Interface Devices (keyboards, mice, etc.)

typedef uint32_t driver_t;

typedef struct driver_info_t
{
    uint32_t type;
    char bin_name[FS_FAT_MAX_FILENAME_LEN];
} __attribute__((packed)) driver_info_t;

// returns information on current (registered) drivers
driver_info_t *driver_get_list(void);

// adds a driver
err_t driver_add(const char *_path, driver_t _type);

// remove driver of _type
err_t driver_remove(driver_t _type);

#endif // __DRIVER_H__
