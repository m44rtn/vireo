/*
MIT license
Copyright (c) 2022 Maarten Vermeulen

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

#ifndef __FILEMAN_H__
#define __FILEMAN_H__

#include "types.h"

#define MAX_FILENAME_LEN 32
#define MAX_PATH_LEN    255

char *fileman_abspath_or_cwd(char *cmd_bfr, const char *abspath, char *cwd);
file_t *read_file_from_bootdisk(const char *path, err_t *err, size_t *fsize);
void merge_disk_id_and_path(const char *disk, const char *path, char *out);
uint8_t fileman_contains_disk(char *path);
uint8_t fileman_is_existing_file(char *path);
err_t setcwd(char *path);
void getcwd(char *buf, uint32_t *len);

#endif // __FILEMAN_H__
