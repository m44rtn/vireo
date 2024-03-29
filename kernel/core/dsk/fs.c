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

#include "fs.h"

#include "diskio.h"
#include "diskdefines.h"
#include "mbr.h"

#include "../memory/memory.h"
#include "../util/util.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../drv/FS_TYPES.H"
#include "../drv/FS_commands.h"
#include "../drv/FS/fs_exitcode.h"

#include "../hardware/driver.h"

#include "../include/exit_code.h"

typedef struct fs_t
{
    syscall_hdr_t hdr;
    char *path;
    file_t *f;
    size_t size;
    char *new_name;
    uint8_t attrib;
} __attribute__((packed)) fs_t;

void fs_api(void *req)
{
    fs_t *fs = (fs_t *) req;
    uint32_t drv[5];

    uint8_t disk_type = drive_type(fs->path);

    if(disk_type == (uint8_t) MAX || !diskio_check_exists(fs->path))
        { fs->hdr.exit_code = EXIT_CODE_FS_UNSUPPORTED_DRIVE; return; }

    switch(fs->hdr.system_call)
    {
        case SYSCALL_GET_FS:
        {
            if(disk_type == DRIVE_TYPE_IDE_PATAPI)
                { fs->hdr.response = FS_TYPE_ISO; break; }
            
            if(disk_type != DRIVE_TYPE_IDE_PATA)
            { fs->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break;}
            
            // if we get here the device is a known type of hard disk
            uint16_t drive_part = drive_convert_drive_id(fs->path);
            fs->hdr.response = mbr_get_type( (uint8_t) (drive_part >> 8), (uint8_t) (drive_part & 0xFF));
            break;
        }

        case SYSCALL_FS_READ:
        {
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

            uint32_t driver_type = (disk_type == DRIVE_TYPE_IDE_PATAPI) ? FS_TYPE_ISO : FS_TYPE_FAT32;

            drv[0] = FS_COMMAND_READ;
            drv[1] = (uint32_t) fs->path;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);
            
            fs->hdr.response_ptr = (void *) drv[2];
            fs->hdr.response_size = drv[3];
            fs->hdr.exit_code = (uint8_t) drv[4];

            break;
        }

        case SYSCALL_FS_WRITE:
        {
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

            uint32_t driver_type = FS_TYPE_FAT32;

            if(disk_type == DRIVE_TYPE_IDE_PATAPI)
            { fs->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break; }

            if(fs->f == NULL)
            {fs->hdr.exit_code = EXIT_CODE_GLOBAL_OUT_OF_RANGE; break;}

            drv[0] = FS_COMMAND_WRITE;
            drv[1] = (uint32_t) fs->path;
            drv[2] = (uint32_t) fs->f;
            drv[3] = (uint32_t) fs->size;
            drv[4] = (uint32_t) fs->attrib;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);
            
            fs->hdr.exit_code = (uint8_t) drv[4];

            break;
        }

        case SYSCALL_FS_DELETE:
        {
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

            uint32_t driver_type = FS_TYPE_FAT32;

            if(disk_type == DRIVE_TYPE_IDE_PATAPI)
            { fs->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break; }

            drv[0] = FS_COMMAND_DELETE;
            drv[1] = (uint32_t) fs->path;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);
            
            fs->hdr.exit_code = (uint8_t) drv[4];

            break;
        }

        case SYSCALL_FS_RENAME:
        {
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

            uint32_t driver_type = FS_TYPE_FAT32;

            if(disk_type == DRIVE_TYPE_IDE_PATAPI)
            { fs->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break; }

            drv[0] = FS_COMMAND_RENAME;
            drv[1] = (uint32_t) fs->path;
            drv[2] = (uint32_t) fs->new_name;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);
            
            fs->hdr.exit_code = (uint8_t) drv[4];

            break;
        }

        case SYSCALL_FS_MKDIR:
        {
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

            uint32_t driver_type = FS_TYPE_FAT32;

            if(disk_type == DRIVE_TYPE_IDE_PATAPI)
            { fs->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break; }

            drv[0] = FS_COMMAND_MKDIR;
            drv[1] = (uint32_t) fs->path;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);
            
            fs->hdr.exit_code = (uint8_t) drv[4];

            break;
        }

        case SYSCALL_FS_GET_FILE_INFO:
        {
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

            uint32_t driver_type = (disk_type == DRIVE_TYPE_IDE_PATAPI) ? FS_TYPE_ISO : FS_TYPE_FAT32;

            drv[0] = FS_COMMAND_GET_FILE_INFO;
            drv[1] = (uint32_t) fs->path;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);
            
            fs->hdr.response_ptr = (void *) drv[2];
            fs->hdr.response_size = sizeof(fs_file_info_t);
            fs->hdr.exit_code = (uint8_t) drv[4];
        }
        break;

        case SYSCALL_FS_GET_DIR_CONTENTS:
        { 
            if(!fs_check_path(fs->path))
                { fs->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }
            
            uint32_t driver_type = (disk_type == DRIVE_TYPE_IDE_PATAPI) ? FS_TYPE_ISO : FS_TYPE_FAT32;

            drv[0] = FS_COMMAND_GET_DIR_CONTENTS;
            drv[1] = (uint32_t) fs->path;
            driver_exec_int(DRIVER_TYPE_FS | driver_type, &drv[0]);

            fs->hdr.response_ptr = (void *) drv[2];
            fs->hdr.response_size = (size_t) drv[3];
            fs->hdr.exit_code = (err_t) drv[4];

            break;
        }

        default:
            fs->hdr.exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;
    }
}

uint8_t fs_check_path(char* p)
{
    if(!p)
        return 0;

    uint32_t fwd_slash_index = find_in_str(p, "/");

    if(fwd_slash_index == MAX)
        return 0;

    return 1; // path is OK
}

file_t *fs_read_file(char *fpath, size_t *o_size)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_READ,
        .path = fpath,
    };
    fs_api(&req);
    
    if(req.hdr.exit_code)
    {
        *o_size = req.hdr.exit_code;
        return NULL;
    }

    *o_size = req.hdr.response_size;
    
    return req.hdr.response_ptr;
}

err_t fs_write_file(char *fpath, file_t *file, size_t fsize, uint8_t attrib)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_WRITE,
        .path = fpath,
        .f = file,
        .size = fsize,
        .attrib = attrib,
    };

    fs_api(&req);
    
    return req.hdr.exit_code;
}

err_t fs_rename_file(char *fpath, char *new_name)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_RENAME,
        .path = fpath,
        .new_name = new_name,
    };

    fs_api(&req);
    
    return req.hdr.exit_code;
}

err_t fs_delete_file(char *fpath)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_DELETE,
        .path = fpath,
    };

    fs_api(&req);
    
    return req.hdr.exit_code;
}

fs_file_info_t *fs_get_file_info(char *fpath, err_t *err)
{
    fs_t req = {
        .hdr.system_call = SYSCALL_FS_GET_FILE_INFO,
        .path = fpath,
    };

    fs_api(&req);

    *err = req.hdr.exit_code;
    return (fs_file_info_t *) req.hdr.response_ptr;
}
