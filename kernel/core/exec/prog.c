/*
MIT license
Copyright (c) 2021-2022 Maarten Vermeulen

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
#include "task.h"
#include "exec.h"

#include "../cpu/interrupts/isr.h"

#include "../include/exit_code.h"

#include "../dbg/dbg.h"

#include "../dsk/fs.h"

#include "../memory/paging.h"
#include "../memory/memory.h"

#include "../kernel/panic.h"

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../util/util.h"

#define PROG_FILENAME_LEN       512
#define PROG_ARG_DELIM          " "
#define PROG_INFO_TABLE_SIZE    4096    // bytes (= 1 page)
#define PROG_INFO_MAX_INDEX     (4096 / sizeof(prog_info_t)) - 1

#define PROG_TERMINATE          0
#define PROG_TERMINATE_STAY     1

#define PROG_FLAG_TERMINATE_STAY PROG_TERMINATE_STAY

#define PROG_FLAG_DRV_RUNNING   1 << 0

typedef struct
{
  pid_t pid;
  pid_t started_by;
  uint8_t flags;
  void *binary_start;
  void *start; // relative start address
  void *stck;
  size_t size;
  char *filename;
  char **argv;
  char *all_args;
} __attribute__((packed)) prog_info_t;

typedef struct api_new_program_t
{
    syscall_hdr_t hdr;
    char *path;
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
    char path[FS_MAX_PATH_LEN];
    void *bin_start;
} __attribute__((packed)) api_prog_info_t;

prog_info_t *prog_info = NULL;
pid_t current_running_pid = PID_KERNEL;
uint8_t prog_flags = 0;

/// ----- ///
uint32_t prog_find_info_index(const pid_t pid);

extern void start(void);
void prog_init(void)
{
    prog_info = evalloc(PROG_INFO_TABLE_SIZE, PID_KERNEL);

    // out of memory?
    if(!prog_info)
        really_easy_panic(PANIC_TYPE_INIT_ERROR, "PROGRAM_EXECUTION_UNSUPPORTED");

    memset((void *) prog_info, PROG_INFO_TABLE_SIZE, 0xFF);

    prog_info[0].start = (void *) ((uint32_t) start);
    prog_info[0].binary_start = (void *) (0x100000u);
    prog_info[0].filename = (char *) "VIREO.SYS";
    prog_info[0].pid = PID_KERNEL;
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

// !! WARNING !!
// Manipulates original string *args!
static err_t prog_parse_argv(char *args, char **argv, uint32_t *argc, char *filename)
{
    // !! WARNING !!
    // Manipulates original string *args!
    *argc = 1;
    uint32_t index = 0;

    if(args[0] == '\0')
        return EXIT_CODE_GLOBAL_INVALID;

    memset(argv, PAGE_SIZE, 0);
    str_get_part(filename, args, PROG_ARG_DELIM, &index);

    index = 0;

    // get argc
    for(; *argc < PAGE_SIZE / sizeof(uint32_t); *argc = *argc + 1u)
    {
        argv[*argc - 1] = &args[index];

        uint32_t res = find_in_str(&args[index], PROG_ARG_DELIM);
        
        if(res == MAX)
            break;

        index += res;

        // null-terminate at space
        args[index] = '\0';
        index++;
    }
    
    // do not allow more than 4095 arguments due to laziness on
    // the programmer's behalf.
    ASSERT((*argc) < PAGE_SIZE / sizeof(uint32_t));
    argv[*argc] = NULL;    
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

static void prog_fill_prog_info(uint32_t free_index, pid_t pid, char *filename, char **argv, char *args, void *rel_addr, void *binary)
{
    void *stack = evalloc(PROG_DEFAULT_STACK_SIZE, pid);

    if(!stack)
        { prog_info[free_index].stck = NULL; return; }

    prog_info[free_index].started_by = current_running_pid;
    prog_info[free_index].filename = filename;
    prog_info[free_index].argv = argv;
    prog_info[free_index].all_args = args;
    prog_info[free_index].pid = pid;
    prog_info[free_index].stck = (void *) (((uint32_t) stack) + PAGE_SIZE - 1U);
    prog_info[free_index].flags = 0;
    prog_info[free_index].start = (void *) ((uint32_t)rel_addr + (uint32_t)(binary));
    prog_info[free_index].binary_start = binary;

    current_running_pid = prog_info[free_index].pid;
}

static void prog_launch_binary_free_buffers(char *filename, char **argv, char *args)
{
    kfree(filename);
    vfree(argv);
    vfree(args);
}

err_t prog_launch_binary(char *arg)
{
    ASSERT(prog_info);

    // find free index in prog_info
    uint32_t free_index = prog_find_free_index();
    ASSERT(free_index);

    if(free_index == MAX)
        return EXIT_CODE_GLOBAL_OUT_OF_RANGE;

    // get a new PID and allocate memory for the arguments and filename of the binary
    pid_t pid = task_new_pid();
    char *filename = kmalloc(512), **argv = evalloc(PAGE_SIZE, pid), *args = evalloc(strlen(arg) + 1, pid);

    if(!filename || !argv || !args)
        { prog_launch_binary_free_buffers(filename, argv, args); return EXIT_CODE_GLOBAL_OUT_OF_MEMORY; }

    memcpy(args, arg, strlen(arg) + 1);
    
    // parse the arguments and fill argv
    uint32_t argc;
    err_t err = prog_parse_argv(args, argv, &argc, filename);

    if(err)
        { prog_launch_binary_free_buffers(filename, argv, args); return err; }

    // read binary file
    size_t size = 0;
    file_t *elf = fs_read_file(filename, &size);

    if(!elf)
        { prog_launch_binary_free_buffers(filename, argv, args); return EXIT_CODE_GLOBAL_GENERAL_FAIL; }

    // parse the ELF
    void *rel_addr = elf_parse_binary(&elf, pid, &err, &prog_info[free_index].size);

    // when err == EXIT_CODE_GLOBAL_GENERAL_FAIL, the binary file is of an unsupported type.
    // otherwise, if err, we listen to whatever elf_parse_binary() is telling us
    if(err && err != EXIT_CODE_GLOBAL_GENERAL_FAIL)
        { prog_launch_binary_free_buffers(filename, argv, args); return err; }
    else if(err && err == EXIT_CODE_GLOBAL_GENERAL_FAIL)
        { prog_launch_binary_free_buffers(filename, argv, args); return EXIT_CODE_GLOBAL_UNSUPPORTED; }

    // save all known information about the program
    prog_fill_prog_info(free_index, pid, filename, argv, args, rel_addr, elf);

    if(!prog_info[free_index].stck)
        { prog_launch_binary_free_buffers(filename, argv, args); return EXIT_CODE_GLOBAL_OUT_OF_MEMORY; }

    // we can finally launch the program!
    err = asm_exec_call(prog_info[free_index].start, prog_info[free_index].stck, argc, argv);

    // we get back here when the program has finished running
    prog_terminate(current_running_pid, (prog_info[free_index].flags & PROG_FLAG_TERMINATE_STAY));

    return err;
}

uint32_t prog_find_info_index(const pid_t pid)
{
    uint32_t i = 0;

    for(; i < PROG_INFO_MAX_INDEX; ++i)
        if(prog_info[i].pid == pid)
            break;

    if(i == PROG_INFO_MAX_INDEX)
        i = MAX;
    
    return i;
}

void prog_set_status_drv_running(void)
{
    prog_flags |= PROG_FLAG_DRV_RUNNING;
}

void prog_set_status_prog_running(void)
{
    prog_flags = 0;
}

uint8_t prog_pid_exists(const pid_t pid)
{
    return (prog_find_info_index(pid) == MAX) ? FALSE : TRUE;
}

pid_t prog_get_current_running(void)
{
    return (prog_flags & PROG_FLAG_DRV_RUNNING) ? PID_DRIVER : current_running_pid;
}

pid_t prog_get_pid_from_eip(uint32_t eip)
{
    for(pid_t i = 0; i < PID_RESV; ++i)
    {
        uint32_t index = prog_find_info_index(i);

        if(index == MAX)
            continue;
        
        uint32_t bin_start = (uint32_t) prog_info[index].binary_start;
        size_t bin_size = (size_t) prog_info[index].size;

        if((eip >= bin_start) && (eip <= (bin_start + bin_size)))
            return i;
    }

    return PID_RESV;
}

const char *prog_get_filename(pid_t pid)
{
    uint32_t index = prog_find_info_index(pid);
    return (const char *) prog_info[index].filename;
}

void *prog_get_binary_start(pid_t pid)
{
    uint32_t index = prog_find_info_index(pid);
    return prog_info[index].binary_start;
}

void prog_set_flags(pid_t pid, uint8_t flags)
{
    uint32_t pid_index = prog_find_info_index(pid);
    prog_info[pid_index].flags |= flags;
}

void prog_terminate(pid_t pid, bool_t stay)
{
    // is the program we are terminating the current program running?
    // (if not, another program is terminating this program)
    uint8_t is_running = (pid == current_running_pid);
    uint32_t pid_index = prog_find_info_index(pid);

    ASSERT(pid_index);
    
    if(pid_index == MAX) 
        return;

    // set new pid to the pid of the program that ran before this program
    // or, in the case that terminate is being called by another program, keep using the current pid
    current_running_pid = (is_running) ? prog_info[pid_index].started_by : current_running_pid;

    if(stay)
        return;
    
    isr_delete_extern_handlers_in_range(prog_info[pid_index].binary_start, (void *) 
                                        ((uint32_t)prog_info[pid_index].binary_start + prog_info[pid_index].size));

    kfree(prog_info[pid_index].filename);
    vfree(prog_info[pid_index].argv);
    vfree(prog_info[pid_index].all_args);
    
    // free binary and stack memory
    vfree((void *) (((uint32_t) prog_info[pid_index].stck) & PAGING_ADDR_MSK));
    vfree(prog_info[pid_index].binary_start);

    // remove information in internal program list
    memset((void *) &prog_info[pid_index], sizeof(prog_info_t), 0xFF);
}

void prog_api(void *req)
{
    syscall_hdr_t *hdr = req;

    ASSERT(prog_info);

    hdr->exit_code = EXIT_CODE_GLOBAL_SUCCESS;

    switch(hdr->system_call)
    {
        case SYSCALL_GET_PROGRAM_INFO:
        {
            api_prog_info_t *info = evalloc(sizeof(api_prog_info_t), current_running_pid);
            ASSERT(info);
            
            uint32_t pid_index = prog_find_info_index(current_running_pid);

            size_t filename_len = strlen(prog_info[pid_index].filename);
            filename_len = ((filename_len + 1u) > FS_MAX_PATH_LEN) ? FS_MAX_PATH_LEN : filename_len + 1u;
            
            memcpy(info->path, prog_info[pid_index].filename, filename_len);
            
            info->pid = current_running_pid;
            info->size = prog_info[pid_index].size;
            info->stack = prog_info[pid_index].stck;
            info->bin_start = prog_info[pid_index].binary_start;

            hdr->response_ptr = info;
            hdr->response_size = sizeof(api_prog_info_t);
            break;
        }

        case SYSCALL_PROGRAM_START_NEW:
        {
            api_new_program_t *n = req;

            n->hdr.exit_code = prog_launch_binary((n->path));
            break;
        }

        case SYSCALL_PROGRAM_TERMINATE_PID:
        {
            api_terminate_t *t = req;
            pid_t pid = (t->pid);

            if(pid == current_running_pid)
                { hdr->exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED; break; }

            if(pid == PID_KERNEL || pid == PID_RESV)
            { hdr->exit_code = EXIT_CODE_GLOBAL_OUT_OF_RANGE; break; }

            prog_terminate(pid, PROG_TERMINATE);
            
            break;
        }

        case SYSCALL_PROGRAM_TERMINATE_STAY:
            prog_set_flags(current_running_pid, PROG_TERMINATE_STAY);
        break;

        default:
            hdr->exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
        break;

    }
}
