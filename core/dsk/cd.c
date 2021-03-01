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

#include "cd.h"
#include "diskdefines.h"
#include "diskio.h"

#include "../hardware/driver.h"

#include "../drv/COMMANDS.H"
#include "../drv/FS_commands.h"
#include "../drv/FS_TYPES.H"

#include "../include/types.h"

#include "../memory/memory.h"


// initializes CD drives (currently only the filesystem driver)
void cd_init(void)
{
    uint8_t *drives = diskio_reportDrives();

    // check if there are any PATAPI drives to begin with
    if(!cd_check_exists(drives))
    { kfree(drives); return; }

    // search for/register the driver
    driver_addInternalDriver((FS_TYPE_ISO | DRIVER_TYPE_FS));
    uint32_t *drv = kmalloc(DRIVER_COMMAND_PACKET_LEN * sizeof(uint32_t));

    for(uint8_t i = 0; i < IDE_DRIVER_MAX_DRIVES; ++i)
    {
        if(drives[i] != DRIVE_TYPE_IDE_PATAPI)
            continue;
        
        drv[0] = DRV_COMMAND_INIT;
        drv[1] = (uint32_t) i;
        
        // ignored by the ISO driver driver
        drv[2] = 0;
        drv[2] = FS_TYPE_ISO;

        driver_exec(FS_TYPE_ISO | DRIVER_TYPE_FS, drv);
    }

    kfree(drv);
    kfree(drives);
}

uint8_t cd_check_exists(uint8_t *drives)
{
    for(uint8_t i = 0; i < IDE_DRIVER_MAX_DRIVES; ++i)
        if(drives[i] == DRIVE_TYPE_IDE_PATAPI)
            return 1; // true
    
    return 0;   // false
}