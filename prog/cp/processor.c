/*
MIT license
Copyright (c) 2022 Maarten Vermeulen

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

#include "util.h"
#include "program.h"

#include "include/commands.h"
#include "include/processor.h"

#define N_INTERNAL_COMMANDS 5
#define CHECK_COMMAND(a) !strcmp_until(a, cmd_bfr, sizeof(a) - 1)

static uint8_t processor_exec_internal_command(char *cmd_bfr)
{
    uint8_t did_execute = 0;

    if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_VER)))
        command_ver();
    
    return did_execute;
}

void processor_execute_command(char *cmd_bfr)
{
    if(processor_exec_internal_command(cmd_bfr))
        return;
    
    uint32_t end = find_in_str(cmd_bfr, "\n");
    cmd_bfr[end] = '\0';

    // TODO: report error in errorlvl command?
    program_start_new(cmd_bfr);
}
