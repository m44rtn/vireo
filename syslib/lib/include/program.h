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

#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "types.h"
#include "call.h"

typedef void stack_t;
typedef uint8_t pid_t;

typedef struct program_info_t
{
    pid_t pid;
    stack_t *stack;
    size_t size;
    char *path;
} __attribute__((packed)) program_info_t;

// returns information about the current program
program_info_t *program_get_info(err_t *err);

// starts a new program located at _path (on disk)
err_t program_start_new(const char *_path, function_t _ret_addr);

// terminates program with _pid (program id), this allows for a 'force termination' function
err_t program_terminate_pid(pid_t _pid);

// terminates current program, without removing it or freeing its resources
err_t program_terminate_stay_resident(void);

#endif // __PROGRAM_H__
