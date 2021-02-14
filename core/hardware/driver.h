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

#ifndef __DRIVER_H__
#define __DRIVER_H__

/* information and indentifier struct for drivers
        take a look in the refman for some of the values */
struct DRIVER
{
    unsigned int sign1;
    char sign2[8];
    unsigned int type;
    unsigned int interface;
} __attribute__ ((packed));

#define DRIVER_TYPE_PCI             0x01 << 24
#define DRIVER_TYPE_FS              0x02 << 24

#define DRIVER_CODE_IDECONTROLLER   DRIVER_TYPE_PCI | 0x0101 /* PCI class 0x01 and subclass 0x01 are for IDE controllers */

/* in DWORDS */
#define DRIVER_COMMAND_PACKET_LEN   5

void driver_init(void);
void driver_exec(unsigned int type, unsigned int *data);
void driver_addInternalDriver(unsigned int identifier);

#endif
