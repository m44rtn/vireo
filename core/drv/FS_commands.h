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

#ifndef __FAT_COMMANDS_H__
#define __FAT_COMMANDS_H__

#define FAT_FILE_ATTRIB_FILE        0x00
#define FAT_FILE_ATTRIB_READONLY    0x01
#define FAT_FILE_ATTRIB_SYSTEM      0x04
#define FAT_FILE_ATTRIB_DIR         0x10

/*#define   FAT_COMMAND_INIT    0x00
---- (not defined since COMMANDS.H already defines INIT)
drv[1] (parameter1) --> drive
drv[2] (parameter2) --> starting LBA of the partition
drv[3] (paramter3) --> FS type (mainly to help drivers out which support multiple
                        filesystems like a FAT driver) */

/* the kernel reserves 0x0F commands as global, so we have to start from 0x10 */
#define FS_COMMAND_READ 0x10
/*
drv[1] (parameter1) --> path
drv[2] (parameter2) --> (returns) pointer to a buffer (driver will return its stuff in this buffer*)
drv[3] (parameter3) --> (returns) size of buffer
drv[4] (parameter4) --> (returns) error code

 * = the software calling this function is supposed to know what kind of thing it wants to read (direcotry/file).

*/

#define FS_COMMAND_WRITE 0x11
/*
drv[1] (parameter1) --> path and filename
drv[2] (parameter2) --> buffer
drv[3] (parameter3) --> size of buffer (file size)
drv[4] (parameter4) --> attributes
drv[4] (parameter4) --> (returns) error code
*/

#define FS_COMMAND_RENAME 0x12
/*
drv[1] (parameter1) --> old path (+ old filename)
drv[2] (parameter2) --> new filename
drv[4] (parameter4) --> (returns) error code
*/

#endif
