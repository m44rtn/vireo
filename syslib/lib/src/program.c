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

#include "../include/program.h"

typedef struct new_program_t
{
    syscall_hdr_t hdr;
    char *path;
    function_t ret_addr;
} __attribute__((packed)) new_program_t;

typedef struct terminate_t
{
    syscall_hdr_t hdr;
    pid_t pid;
} __attribute__((packed)) terminate_t;

program_info_t *program_get_info(err_t *err)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_PROGRAM_INFO};
    asm_syscall(&hdr);

    *err = hdr.exit_code;
    
    return (program_info_t *) hdr.response_ptr;
}

err_t program_start_new(const char *_path, function_t _ret_addr)
{
    new_program_t req = {
        .hdr.system_call = SYSCALL_PROGRAM_START_NEW,
        .path = (char *) (_path),
        .ret_addr = _ret_addr
    };
    asm_syscall(&req);

    return req.hdr.exit_code;
}

err_t program_terminate(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_PROGRAM_TERMINATE};
    asm_syscall(&hdr);

    return hdr.exit_code;
}

err_t program_terminate_pid(pid_t _pid)
{
    terminate_t req = {
        .hdr.system_call = SYSCALL_PROGRAM_TERMINATE_PID,
        .pid = _pid
    };
    asm_syscall(&req);

    return req.hdr.exit_code;
}

err_t program_terminate_stay_resident(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_PROGRAM_TERMINATE_STAY};
    asm_syscall(&hdr);

    return hdr.exit_code;
}
