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

#include "../include/disk.h"

typedef struct disk_syscall_t
{
    syscall_hdr_t hdr;
    char *drive;
    uint32_t lba;
    uint32_t nlba;
    size_t buffer_size;
    void *buffer;
} __attribute__((packed)) disk_syscall_t;


disk_info_t *disk_get_drive_list(size_t *size)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_DISK_LIST};
    PERFORM_SYSCALL(&hdr);

    *(size) = hdr.response_size;

    return (disk_info_t *) hdr.response_ptr;
}

partition_info_t *disk_get_partition_info(char *_id)
{
    disk_syscall_t req = {
        .hdr.system_call = SYSCALL_PARTITION_INFO,
        .drive = _id
    };

    PERFORM_SYSCALL(&req);

    return (partition_info_t *) req.hdr.response_ptr;
}

void *disk_absolute_read(char *_drive, uint32_t _lba, uint32_t _sctrs)
{
    disk_syscall_t req = {
        .hdr.system_call = SYSCALL_DISK_ABS_READ,
        .drive = _drive,
        .lba = _lba,
        .nlba = _sctrs
    };

    PERFORM_SYSCALL(&req);

    return req.hdr.response_ptr;
}
 
err_t disk_absolute_write(char *_drive, uint32_t _lba, void *_bfr, size_t _bfr_size)
{
    disk_syscall_t req = {
        .hdr.system_call = SYSCALL_DISK_ABS_WRITE,
        .drive = _drive,
        .lba = _lba,
        .buffer = _bfr,
        .buffer_size = _bfr_size
    };

    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}

char *disk_get_bootdisk(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_DISK_GET_BOOTDISK};
    PERFORM_SYSCALL(&hdr);

    return (char *) hdr.response_ptr;
}
