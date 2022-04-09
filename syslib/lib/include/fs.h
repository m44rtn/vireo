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

#ifndef __FS_H__
#define __FS_H__

#include "types.h"
#include "call.h"

#define FS_TYPE_FAT12   0x01
#define FS_TYPE_FAT16   0x04
#define FS_TYPE_FAT32   0x0B

#define FS_TYPE_ISO     0x96

typedef void file_t;

// returns the filesystem type of the drive specified
uint8_t fs_get_filesystem(char *_drive);

// returns the file located at _path, returning the file size in _o_size
file_t *fs_read_file(char *_path, size_t *_o_size, err_t *err);

// writes file to _path
err_t fs_write_file(char *_path, file_t *_file, size_t _size);

// removes file at _path
err_t fs_delete_file(char *_path);

// renames file at _path to _new_name
err_t fs_rename_file(char *_path, char *_new_name);

#endif // __FS_H__
