/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

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

#include "fat.h"

#include "../../include/types.h"

#include "../../hardware/driver.h"

#include "../COMMANDS.H"
#include "../FS_TYPES.H"

#define FAT_FS_ID32 0x0B

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t Ptype, uint32_t Dtype);
void FAT_HANDLER(uint32_t *drv);

/* the indentifier for drivers + information about our driver */
struct DRIVER FAT_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_FAT | DRIVER_TYPE_FS), (uint32_t) (FAT_HANDLER)};

/* FAT12 and 16 COMING SOON */

void FAT_HANDLER(uint32_t *drv)
{
    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
        FAT_init(drv[1], drv[2], drv[3], drv[4]);
        break;
    }
}

static void FAT_init(uint32_t drive, uint32_t partition, uint32_t Ptype, uint32_t Dtype)
{

    /* todo:
      - save drive to info list (don't forget to check if we've already saved it)
      - detect fat type
      - save fat type to info list
      - get all necessarry info about the fs
      - print hello -- ifndef NO_DEBUG_INFO of course */

}
