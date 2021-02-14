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

#ifndef __IDE_COMMANDS_H__
#define __IDE_COMMANDS_H__


/*#define IDE_COMMAND_INIT    0x00
---- (not defined since COMMANDS.H already defines INIT)
    Paramaters for the INIT command to the IDE driver are the following:
        parameter1: internal PCI device ID (bus, device, function, class)
        parameter2, parameter3, parameter4
*/


/* commands 0x00 - 0x0f are reserved for global things so that's why we use 0x10 here */

#define IDE_COMMAND_READ    0x10
/*
	parameter1: drive
	parameter2: starting sector
	parameter3: # sectors to read
	parameter4: buffer to read to
*/

#define IDE_COMMAND_WRITE   0x11
/*
	parameter1: drive
	parameter2: starting sector
	parameter3: # sectors to write
	parameter4: buffer with the data to be written
*/

#define IDE_COMMAND_REPORTDRIVES   0x12
/*
reports the drive 'map' detected by the driver in an array as large as IDE_DRIVER_MAX_DRIVES:

array[0] will be the first drive slot
array[1] will be the second drive slot
etc.

the array uses the defines listed below (1) to indicate what drive type was found at which slot.
 unknown means no drive attached, in most cases.

parameter1: pointer to array to store the map in

(1):
DRIVE_TYPE_IDE_PATA
DRIVE_TYPE_IDE_PATAPI
DRIVE_TYPE_UNKNOWN
*/

#ifndef IDE_DRIVER_MAX_DRIVES
#define IDE_DRIVER_MAX_DRIVES   4
#endif

#endif
