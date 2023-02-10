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

#include "include/fileman.h"

#include "disk.h"
#include "memory.h"
#include "util.h"
#include "fs.h"

char working_dir[MAX_PATH_LEN + 1];

file_t *read_file_from_bootdisk(const char *path, err_t *err, size_t *fsize)
{
    char *bootdisk = disk_get_bootdisk();
    char *opath = valloc(strlen(bootdisk) + strlen(path) + 1);

    merge_disk_id_and_path(bootdisk, (char *) path, opath);
    vfree(bootdisk);

    file_t *f = fs_read_file(opath, fsize, err);
    vfree(opath);

    return f;
}

void merge_disk_id_and_path(char *disk, char *path, char *out)
{    
    size_t d_len = strlen(disk);
    memcpy(out, disk, d_len);
    memcpy(&out[d_len], path, strlen(path) + 1);
}

uint8_t fileman_contains_disk(char *path)
{
    // not happy with this, but it works
    if(!(path[2] >= '0' && path[2] <= '9'))
        return 0;
    
    if(!(path[1] >= 'A' && path[1] <= 'Z'))
        return 0;
    
    if(!(path[0] >= 'A' && path[0] <= 'Z'))
        return 0;

    return 1;    
}

static uint8_t fileman_is_existing_dir(char *path)
{
    err_t err = 0;
    uint8_t res = 1;
    fs_file_info_t *t = fs_file_get_info(path, &err);

    if(!t)
        res = 0;
    else if(err)
        res = 0;
    else if((t->file_type & FAT_FILE_ATTRIB_DIR) != FAT_FILE_ATTRIB_DIR)
       res = 0;
    
    vfree(t);
    return res;
}

err_t setcwd(char *path)
{
    // first check if this path even exists...
    if(!fileman_is_existing_dir(path))
        return EXIT_CODE_GLOBAL_INVALID;

    size_t len = strlen(path) + 1;
    len = (len > MAX_PATH_LEN + 1) ? MAX_PATH_LEN + 1 : len;

    memcpy(working_dir, path, len);

    if(working_dir[len - 2] != '/')
        { working_dir[len - 1] = '/'; working_dir[len] = '\0'; }
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

void getcwd(char *buf, uint32_t *len)
{
    *len = strlen(working_dir);
    memcpy(buf, working_dir, *len + 1);
}
