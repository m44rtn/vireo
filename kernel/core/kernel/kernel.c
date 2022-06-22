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

#include "kernel.h"
#include "info.h"

#include "../dbg/debug.h"

#include "../include/exit_code.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../util/util.h"

#include "../memory/memory.h"
#include "../memory/paging.h"

#include "../cpu/interrupts/isr.h"

#include "../hardware/timer.h"

#include "../exec/task.h"
#include "../exec/prog.h"
#include "../dsk/fs.h"
#include "../dsk/bootdisk.h"

#include "../main.h"

#define CONFIG_FILE_PATH_LEN    512 // bytes, maximum
#define DEFAULT_CONFIG_FILE_LOC "/CONFIG"

#define KERNEL_REV_MAX_SIZE     8

typedef struct int_request_t
{
    syscall_hdr_t hdr;
    void *handler;
    uint8_t intr;
} __attribute__((packed)) int_request_t;

typedef struct sleep_request_t
{
    syscall_hdr_t hdr;
    uint32_t ms;
} __attribute__((packed)) sleep_request_t;

typedef struct kernel_ver_t
{
    uint16_t major;
    uint16_t minor;
    uint32_t build;
    char rev[KERNEL_REV_MAX_SIZE];
} __attribute__((packed)) kernel_ver_t;

void kernel_api_handler(void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *) req;

    switch(hdr->system_call)
    {
        case SYSCALL_VERSION_STR:
        {
            // full version string
            char *str = info_make_version_str();
            
            hdr->response_ptr = evalloc(strlen(str) + 1, prog_get_current_running());
            hdr->response_size = strlen(str) + 1;
            memcpy((hdr->response_ptr), str, strlen(str) + 1);
            kfree(str);
            break;
        }

        case SYSCALL_VERSION_NUM:
        {
            // version number (build, major, minor, rev)
            kernel_ver_t *ver = evalloc(sizeof(kernel_ver_t), prog_get_current_running());
            hdr->response_ptr = ver;
            hdr->response_size = sizeof(kernel_ver_t);

            ver->build = BUILD;
            ver->major = MAJOR;
            ver->minor = MINOR;

            memcpy(&(ver->rev), REV, strlen(REV));
            break;
        }

        case SYSCALL_FREE_INT_HANDLERS:
            hdr->response_ptr = evalloc(MAX_PIC_INTERRUPTS * sizeof(void *), prog_get_current_running());
            hdr->response_size = MAX_PIC_INTERRUPTS * sizeof(void *);
            memcpy(hdr->response_ptr, isr_get_extern_handlers(), sizeof(void *));
        break;

        case SYSCALL_ADD_INT_HANDLER:
        {
            int_request_t *intr = (int_request_t *) req;
            isr_set_extern_handler(intr->intr, intr->handler);
            break;
        }

          case SYSCALL_GET_SYSTICKS:
            hdr->response_ptr = (void *) timer_getCurrentTick();
        break;
        
        case SYSCALL_SLEEP:
        {
            sleep_request_t *r = (sleep_request_t *) req;
            sleep(r->ms);    
            break;
        }

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;

    }
}

void kernel_fetch_new_line(file_t *f, size_t size, uint32_t *loc, char *bfr)
{
    char *file = f;
    uint32_t i = *(loc), st = *(loc); // inc, start

    // copy until '\n' 
    for(; i < size; ++i)
    {
        if(file[i] == '\n')
            break;

        bfr[i - st] = file[i];
    }

    *(loc) = i + 1;
}

void kernel_line_strip(char *line)
{
    for(uint32_t i = 0; i < CONFIG_FILE_PATH_LEN; ++i)
    {
        // if part of file path, skip
        if( (line[i] >= 'A' && line[i] <= 'Z') || (line[i] >= '0' && line[i] <= '9') || line[i] == '.' || line[i] == '/')
            continue;

        // otherwise cut the string right here
        line[i] = '\0';
        break;
    }
}

char *kernel_parse_config(file_t *f, size_t fsize)
{
    uint32_t loc = 0;
    char *line = kmalloc(CONFIG_FILE_PATH_LEN);

    while(loc < fsize)
    {
        kernel_fetch_new_line(f, fsize, &loc, line);

        if(line[0] == '#')
            continue;
        else if(line[0] < '/' || line[0] > 'z')
            continue;
        else
            break;
    }
    
    kernel_line_strip(line);

    if(loc > fsize || !strlen(line))
    {
        kfree(line);
        debug_print_error("Error parsing configuration file");
        return NULL;
    }

    return line;
}

void kernel_execute_config(void)
{
    char *config_file = kmalloc(CONFIG_FILE_PATH_LEN);
    char *disk = bootdisk();
    
    // create file path of config file
    memcpy(config_file, disk, strlen(disk));
    memcpy(&config_file[strlen(config_file)], (char *) DEFAULT_CONFIG_FILE_LOC, strlen(DEFAULT_CONFIG_FILE_LOC));

    // read config file
    size_t size = 0;

    // FIXME: f is not freed before launching the program
    file_t *f = fs_read_file(config_file, &size);
    kfree(config_file);

    // parse config file
    char *program = kernel_parse_config(f, size);
    
    // error parsing config file
    if(!program)
    { kfree(disk); return; }

    // if the config file only lists a path that starts at root 
    // (no disk specified), we need to add the bootdisk in front
    // of the path
    if(program[0] == '/')
    {
        move_str_back(program, strlen(disk));
        memcpy(program, disk, strlen(disk));
    }

    kfree(disk);

    if(prog_launch_binary(program))
        debug_print_error("Could not execute program in CONFIG");
}
