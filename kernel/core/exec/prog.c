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

#include "prog.h"

#include "elf.h"
#include "flat.h"
#include "task.h"
#include "exec.h"

#include "../include/exit_code.h"

#include "../dbg/dbg.h"

#include "../dsk/fs.h"

#include "../memory/paging.h"

#include "../kernel/panic.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../util/util.h"

#define PROG_INFO_TABLE_SIZE    4096    // bytes (= 1 page)
#define PROG_INFO_MAX_INDEX     (4096 / sizeof(prog_info_t)) - 1

#define PROG_TERMINATE          0
#define PROG_TERMINATE_STAY     1

typedef struct
{
  pid_t pid;
  pid_t started_by;
  void *rel_start; // relative start address
  void *stck;
  size_t size;     // FIXME: currently file size not size in memory
  char *filename;
  return_t ret_addr;
} __attribute__((packed)) prog_info_t;

typedef struct api_new_program_t
{
    syscall_hdr_t hdr;
    char *path;
    return_t ret_addr;
} __attribute__((packed)) api_new_program_t;

typedef struct terminate_t
{
    syscall_hdr_t hdr;
    pid_t pid;
} __attribute__((packed)) api_terminate_t;

typedef struct program_info_t
{
    pid_t pid;
    void *stack;
    size_t size;
    char *path;
} __attribute__((packed)) api_prog_info_t;

prog_info_t *prog_info = NULL;
pid_t current_running_pid = PID_KERNEL;

/// ----- ///
void prog_init(void);
uint32_t prog_find_info_index(const pid_t pid);

void prog_init(void)
{
    prog_info = evalloc(PROG_INFO_TABLE_SIZE, PID_KERNEL);

    // out of memory?
    if(!prog_info)
        really_easy_panic(PANIC_TYPE_EXCEPTION, "PROGRAM_EXECUTION_UNSUPPORTED");

    memset((void *) prog_info, PROG_INFO_TABLE_SIZE, 0xFF);
}

uint32_t prog_find_free_index(void)
{
    uint32_t i = 0;
    for(; i < PROG_INFO_MAX_INDEX; ++i)
        if(prog_info[i].pid == 0xFF)
            break;
    if(i == PROG_INFO_MAX_INDEX) 
        i = MAX;

    return i;
}

void prog_launch_binary(char *filename, return_t ret_addr)
{
    if(!prog_info)
        prog_init();

    // find free index in prog_info
    uint32_t free_index = prog_find_free_index();

    if(free_index == MAX)
        return;

    size_t size = 0;

    // read binary file
    file_t *f = fs_read_file(filename, &size);
    file_t *elf = f;

    // save all known information about the program
    prog_info[free_index].size = size; // file size (not size in memory)
    prog_info[free_index].started_by = current_running_pid;
    prog_info[free_index].filename = filename; // FIXME: could point to an unkown program's memory
    prog_info[free_index].ret_addr = ret_addr;
    prog_info[free_index].pid = task_new_pid();
    prog_info[free_index].stck = (void *) (((uint32_t)evalloc(PROG_DEFAULT_STACK_SIZE, prog_info[free_index].pid)) + PAGE_SIZE - 1U);

    err_t err = 0;
    void *rel_addr = elf_parse_binary(&elf, prog_info[free_index].pid, &err);

    if(!rel_addr)
    {
        paging_rel_resources(prog_info[free_index].pid);
        memset((void *) &prog_info[free_index], sizeof(prog_info_t), 0xFF);
        return;
    }

    if(!err)
        prog_info[free_index].rel_start = (void *) ((uint32_t)rel_addr | (uint32_t)(elf));
    else 
        prog_info[free_index].rel_start = f; // start of file (in case of flat binary)

    current_running_pid = prog_info[free_index].pid;

    asm_exec_call(prog_info[free_index].rel_start, prog_info[free_index].stck);
}

uint32_t prog_find_info_index(const pid_t pid)
{
    uint32_t i = 0;

    for(; i < PROG_INFO_MAX_INDEX; ++i)
    {
        if(prog_info[i].pid == pid)
            break;
        else if(i == PROG_INFO_MAX_INDEX)
            { i = MAX; break; } // if still not found when i == max_index 
                                // then use MAX to indicate NOT FOUND
    }
    
    return i;
}

uint8_t prog_pid_exists(const pid_t pid)
{
    return (prog_find_info_index(pid) == MAX) ? FALSE : TRUE;
}

pid_t prog_get_current_running(void)
{
    return current_running_pid;
}

const char *prog_get_filename(pid_t pid)
{
    uint32_t index = prog_find_info_index(pid);
    return (const char *) prog_info[index].filename;
}

void prog_terminate(pid_t pid, bool_t stay)
{
    // is the program we are terminating the current program running?
    uint8_t is_running = (pid == current_running_pid);
    
    uint32_t pid_index = prog_find_info_index(pid);
    
    if(pid_index == MAX) 
        return;

    // set new pid to the pid of the program that ran before this program
    // or, in the case that terminate is being called by another program, keep using the current pid
    current_running_pid = (is_running) ? prog_info[pid_index].started_by : current_running_pid;
    return_t ret_addr = prog_info[pid_index].ret_addr;
    
    // in case of TERMINATE_STAY, do not release the resources of the program
    if(!stay)
    {
        paging_rel_resources(pid);
        memset((void *) &prog_info[pid_index], sizeof(prog_info_t), 0xFF);
    }

    // if the current program is terminating another program
    // then we don't need to go back to the program before the current program.
    if(is_running)
    {
        pid_index = prog_find_info_index(current_running_pid);

        // FIXME?: `wipes` pervious stack pointer of that program
        asm_exec_call((void *) ret_addr, prog_info[pid_index].stck);
    }
}

void prog_api(void *req)
{
    syscall_hdr_t *hdr = req;

    if(!prog_info)
        prog_init();

    hdr->exit_code = EXIT_CODE_GLOBAL_SUCCESS;

    switch(hdr->system_call)
    {
        case SYSCALL_GET_PROGRAM_INFO:
        {
            api_prog_info_t *info = evalloc(sizeof(api_prog_info_t), current_running_pid);
            dbg_assert(info);
            
            uint32_t pid_index = prog_find_info_index(current_running_pid);
            
            // FIXME: copy filename instead of giving the pointer to it
            info->path = prog_info[pid_index].filename;
            info->pid = current_running_pid;
            info->size = prog_info[pid_index].size;
            info->stack = prog_info[pid_index].stck;

            hdr->response_ptr = info;
            hdr->response_size = sizeof(api_prog_info_t);
            break;
        }

        case SYSCALL_PROGRAM_START_NEW:
        {
            api_new_program_t *n = req;
            prog_launch_binary((n->path), (n->ret_addr));
            break;
        }

        case SYSCALL_PROGRAM_TERMINATE:
            prog_terminate(current_running_pid, PROG_TERMINATE);
        break;

        case SYSCALL_PROGRAM_TERMINATE_PID:
        {
            api_terminate_t *t = req;
            pid_t pid = (t->pid);

            if(pid == PID_KERNEL || pid == PID_RESV)
            { hdr->exit_code = EXIT_CODE_GLOBAL_OUT_OF_RANGE; break; }

            prog_terminate(pid, PROG_TERMINATE);
            
            break;
        }

        case SYSCALL_PROGRAM_TERMINATE_STAY:
            prog_terminate(current_running_pid, PROG_TERMINATE_STAY);
        break;

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;

    }
}
