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

#include "IDEController.h"

#include "../include/types.h"
#include "../screen/screen_basic.h"
#include "../hardware/driver.h"

/* the indentifier for drivers + information about our driver */
struct DRIVER drv = {(uint32_t) 0xB14D05, "VIREODRV", (uint32_t) IDEControllerInit};

uint32_t device_list[4];
uint8_t registered_devices = 0; /* amount of devices registered, isn't the nicest solution */


/*const uint16_t sign1 = (const uint16_t) 0xB14D05; /* 'BirdOS' 
const char *sign2 = "VIREODRV";
const uint32_t interface = (uint32_t) hello_world;*/

void IDEControllerInit(uint32_t device)
{
    if(registered_devices < 3)
        device_list[registered_devices] = device;
}
