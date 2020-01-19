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

#include "IDEController.h"

#include "../../include/types.h"
#include "../../screen/screen_basic.h"
#include "../../hardware/driver.h"

#define IDEController_PCI_CLASS_SUBCLASS    0x101

/* the indentifier for drivers + information about our driver */
struct DRIVER drv = {(uint32_t) 0xB14D05, "VIREODRV", (IDEController_PCI_CLASS_SUBCLASS | DRIVER_TYPE_PCI),(uint32_t *) IDEController_handler};

uint32_t device_list[4];
uint8_t registered_devices = 0; /* amount of devices registered, isn't the nicest solution */

static void IDEControllerInit(unsigned int device);


void IDEController_handler(uint32_t *data)
{
    trace("[IDE_DRIVER] passed argument: %s\n", data);
}

static void IDEControllerInit(uint32_t device)
{
    if(registered_devices < 3)
        device_list[registered_devices] = device;

    trace("[IDE_DRIVER] Kernel registered device: 0x%x\n", device);
}
