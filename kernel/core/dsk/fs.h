/*
MIT license
Copyright (c) 2019-2022 Maarten Vermeulen

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

#include "../include/types.h"
#include "../drv/FS_TYPES.H"

#define FS_MAX_PATH_LEN     255

void fs_api(void *req);
uint8_t fs_check_path(char* p);
file_t *fs_read_file(char *fpath, size_t *o_size);

err_t fs_write_file(char *fpath, file_t *file, size_t fsize, uint8_t attrib);
err_t fs_rename_file(char *fpath, char *new_name);
err_t fs_delete_file(char *fpath);
fs_file_info_t *fs_get_file_info(char *fpath, err_t *err);

#endif
