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

#include "../include/fs.h"

typedef struct fs_t
{
    syscall_hdr_t hdr;
    char *path;
    file_t *f;
    size_t size;
    char *new_name;
    uint8_t attrib;
} __attribute__((packed)) fs_t;

uint8_t fs_get_filesystem(char *_drive)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_GET_FS,
        .path = _drive
    };
    asm_syscall(&req);

    return (uint8_t) (req.hdr.response);
}

file_t *fs_read_file(char *_path, size_t *_o_size, err_t *err)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_READ,
        .path = _path
    };
    asm_syscall(&req);

    *_o_size = req.hdr.response_size;
    *err = req.hdr.exit_code;

    return (file_t *) req.hdr.response_ptr;
}

err_t fs_write_file(char *_path, file_t *_file, size_t _size, uint8_t _attrib)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_WRITE,
        .path = _path,
        .f = _file,
        .size = _size,
        .attrib = _attrib,
    };
    asm_syscall(&req);

    return req.hdr.exit_code;
}

err_t fs_delete_file(char *_path)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_DELETE,
        .path = _path
    };
    asm_syscall(&req);

    return req.hdr.exit_code;
}

err_t fs_rename_file(char *_path, char *_new_name)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_RENAME,
        .path = _path,
        .new_name = _new_name
    };
    asm_syscall(&req);

    return req.hdr.exit_code;
}

err_t fs_mkdir(char *_path)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_MKDIR,
        .path = _path,
    };
    asm_syscall(&req);

    return req.hdr.exit_code;
}

fs_file_info_t *fs_file_get_info(char *_path, err_t *_err)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_GET_FILE_INFO,
        .path = _path,
    };
    asm_syscall(&req);

    *_err = req.hdr.exit_code;
    return req.hdr.response_ptr;
}
