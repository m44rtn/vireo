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

#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

// kernel (0x0000-0x00ff)
#define SYSCALL_VERSION_STR                 0x0000
#define SYSCALL_VERSION_NUM                 0x0001
#define SYSCALL_GET_INT_HANDLERS_FROM_NUM   0x0010
#define SYSCALL_ADD_INT_HANDLER             0x0011
#define SYSCALL_REM_INT_HANDLER             0x0012
#define SYSCALL_GET_SYSTICKS                0x0020
#define SYSCALL_SLEEP                       0x0021

// screen (0x0100-0x01ff)
#define SYSCALL_GET_SCREEN_INFO             0x0100
#define SYSCALL_PRINT                       0x0101
#define SYSCALL_PRINT_AT                    0x0102
#define SYSCALL_GET_SCREEN_BUFFER           0x0103
#define SYSCALL_GET_SCREEN_GET_BYTE         0x0104
#define SYSCALL_SET_SCREEN_COLOR            0x0105
#define SYSCALL_CLEAR_SCREEN                0x0106
#define SYSCALL_SET_SCREEN_CURSOR           0x0107
#define SYSCALL_GET_SCREEN_CURSOR           0x0108

// memory (0x0200-0x02ff)
#define SYSCALL_GET_MEM_INFO                0x0200
#define SYSCALL_VALLOC                      0x0201
#define SYSCALL_VFREE                       0x0202

// disk absolute (0x0300-0x03ff)
#define SYSCALL_DISK_LIST                   0x0300
#define SYSCALL_PARTITION_INFO              0x0301
#define SYSCALL_DISK_ABS_READ               0x0302
#define SYSCALL_DISK_ABS_WRITE              0x0303
#define SYSCALL_DISK_GET_BOOTDISK           0x0304

// filesystem (0x0400-0x04ff)
#define SYSCALL_GET_FS                      0x0400
#define SYSCALL_FS_READ                     0x0401
#define SYSCALL_FS_WRITE                    0x0402
#define SYSCALL_FS_DELETE                   0x0403
#define SYSCALL_FS_RENAME                   0x0404
#define SYSCALL_FS_MKDIR                    0x0405
#define SYSCALL_FS_GET_FILE_INFO            0x0406

// program (0x0500-0x05ff)
#define SYSCALL_GET_PROGRAM_INFO            0x0500
#define SYSCALL_PROGRAM_START_NEW           0x0501
#define SYSCALL_PROGRAM_TERMINATE_PID       0x0502
#define SYSCALL_PROGRAM_TERMINATE_STAY      0x0503 // terminate stay resident

// driver (0x0a00-0x0aff)
#define SYSCALL_DRIVER_GET_LIST             0x0a00
#define SYSCALL_DRIVER_ADD                  0x0a01
#define SYSCALL_DRIVER_REMOVE               0x0a02

// api (0x0b00-0x0bff)
#define SYSCALL_GET_API_LISTING             0x0b00
#define SYSCALL_REQUEST_API_SPACE           0x0b01
#define SYSCALL_FREE_API_SPACE              0x0b02

// debug (0x0c00-0x0cff)
#define SYSCALL_NOP                         0x0c00


#endif // __SYSCALLS_H__
