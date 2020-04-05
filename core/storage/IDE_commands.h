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

#ifndef __IDE_COMMANDS_H__
#define __IDE_COMMANDS_H__


#define IDE_COMMAND_INIT    0x00
/*
    Paramaters for the INIT command to the IDE driver are the following:
        parameter1: internal PCI device ID (bus, device, function, class)
        parameter2, parameter3, parameter4
*/

#define IDE_COMMAND_READ    0x01
/*
	parameter1: drive
	parameter2: starting sector
	parameter3: # sectors to read
	parameter4: buffer to read to
*/

#define IDE_COMMAND_WRITE    0x02
/*
	parameter1: drive
	parameter2: starting sector
	parameter3: # sectors to write
	parameter4: buffer with the data to be written
*/

#endif
