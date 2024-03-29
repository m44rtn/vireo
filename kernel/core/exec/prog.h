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

#ifndef __PROG_H__
#define __PROG_H__

#include "../include/types.h"

#define PROG_DEFAULT_STACK_SIZE 4096    // bytes

void prog_init(void);
void prog_api(void *req);
err_t prog_launch_binary(char *filename);
uint8_t prog_pid_exists(const pid_t pid);
uint32_t prog_find_free_index(void);

void prog_set_status_drv_running(void);
void prog_set_status_prog_running(void);

pid_t prog_get_current_running(void);
pid_t prog_get_pid_from_eip(uint32_t eip);
const char *prog_get_filename(pid_t pid);
void *prog_get_binary_start(pid_t pid);
void prog_set_flags(pid_t pid, uint8_t flags);
void prog_terminate(pid_t pid, bool_t stay);

#endif // __PROG_H__
