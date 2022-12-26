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
#include "../dsk/diskio.h"

#include "../main.h"

#define CONFIG_FILE_PATH_LEN    512 // bytes, maximum
#define DEFAULT_CONFIG_FILE_LOC "/CONFIG"

#define KERNEL_REV_MAX_SIZE     8

#define ERROR_PARSING_CONFIG    "Error parsing config file (error code 0x%x)"
#define ERROR_EXEC_PROG         "Error executing program in CONFIG (error code 0x%x)"

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

uint8_t g_kernel_flags = 0;

void kernel_api_handler(void *req)
{
    syscall_hdr_t *hdr = (syscall_hdr_t *) req;

    switch(hdr->system_call)
    {
        case SYSCALL_VERSION_STR:
        {
            // full version string
            char *str = info_make_version_str();
            
            hdr->response_ptr = str;
            hdr->response_size = strlen(str) + 1;
            memcpy((hdr->response_ptr), str, strlen(str) + 1);
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

        case SYSCALL_GET_INT_HANDLERS_FROM_NUM:
        {
            int_request_t *intr = (int_request_t *) req;
            uint32_t max_entries = isr_max_extern_handlers();

            intr->hdr.response_size = max_entries * sizeof(void *);
            intr->hdr.response_ptr = evalloc(hdr->response_size, prog_get_current_running());
            
            uint32_t actual_entries = isr_get_extern_handlers(intr->intr, (void **) intr->hdr.response_ptr, intr->hdr.response_size);

            if(actual_entries == MAX)
                intr->hdr.exit_code = EXIT_CODE_GLOBAL_GENERAL_FAIL;

            intr->hdr.response_size = actual_entries * sizeof(void *);
        }
        break;

        case SYSCALL_ADD_INT_HANDLER:
        {
            int_request_t *intr = (int_request_t *) req;
            isr_set_extern_handler(intr->intr, intr->handler);
            break;
        }

        case SYSCALL_REM_INT_HANDLER:
        {
            int_request_t *intr = (int_request_t *) req;
            isr_delete_extern_handler(intr->intr, intr->handler);
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

    // null terminate copied string
    bfr[i - st] = '\0';
}

// FIXME: not used currently, maybe remove if still not used by v0.1
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

static void kernel_report_error(err_t err, const char *error_text_format)
{
    char s[64];
    str_add_val(s, error_text_format, err);
    debug_print_error(s);
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

    if(loc > (fsize + 1u) || !strlen(line))
    {
        kfree(line);
        kernel_report_error(EXIT_CODE_GLOBAL_INVALID, ERROR_PARSING_CONFIG);
        return NULL;
    }

    return line;
}

err_t kernel_format_program_path(char *p)
{    
    uint16_t drive = drive_convert_drive_id(p);
    uint32_t fwd_slash_index = find_in_str(p, "/");

    if(drive != (uint16_t)MAX && fwd_slash_index != MAX)
        return EXIT_CODE_GLOBAL_SUCCESS;
    else if(drive != (uint16_t)MAX && fwd_slash_index == MAX)
        return EXIT_CODE_GLOBAL_INVALID;
    
    if(p[0] != '/' && drive == (uint16_t)MAX)
    { 
        move_str_back(p, 1); 
        p[0] = '/'; 
    }

    return EXIT_CODE_GLOBAL_SUCCESS;
}

void kernel_execute_config(void)
{
    char *config_file = kmalloc(CONFIG_FILE_PATH_LEN);
    char *disk = bootdisk();
    
    // create file path of config file
    memcpy(config_file, disk, strlen(disk) + 1);
    memcpy(&config_file[strlen(config_file)], (char *) DEFAULT_CONFIG_FILE_LOC, strlen(DEFAULT_CONFIG_FILE_LOC));

    // read config file
    size_t size = 0;

    file_t *f = fs_read_file(config_file, &size);
    kfree(config_file);

    // parse config file
    char *program = kernel_parse_config(f, size);
    vfree(f);
    
    // error parsing config file
    if(!program)
    { kfree(disk); return; }

    err_t format_err = kernel_format_program_path(program);
    
    if(format_err)
        kernel_report_error(EXIT_CODE_GLOBAL_INVALID, ERROR_PARSING_CONFIG);

    // if the config file only lists a path that starts at root 
    // (no disk specified), we need to add the bootdisk in front
    // of the path
    if(program[0] == '/')
    {
        move_str_back(program, strlen(disk));
        memcpy(program, disk, strlen(disk));
    }

    kfree(disk);

    err_t err = prog_launch_binary(program);
    if(err)
        kernel_report_error(err, ERROR_EXEC_PROG);
}
