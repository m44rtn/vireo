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

#include "types.h"
#include "exit_code.h"
#include "memory.h"
#include "util.h"
#include "screen.h"

#include "include/config.h"
#include "include/keyb.h"
#include "include/processor.h"

#define PROMPT  "$ "
#define COMMAND_BUFFER_SIZE 512 // chars

static void print_did_not_exec_correctly(char *cmd_bfr)
{
    char str[512];
    str_add_val(str, "%s: no command or filename, or program returned with error.\n", cmd_bfr);
    screen_print(str);
}

err_t main(uint32_t argc, char **argv)
{    
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    file_t *cf = config_read_file(&err); 

    if(err)
        return err;

    err = config_load_drv(cf);

    if(err)
        return err;
    
    err = keyb_start(cf);
    
    if(err)
        return err;
    
    char *cmd_bfr = valloc(COMMAND_BUFFER_SIZE);
    uint32_t i = 0;

    screen_print(PROMPT);

    while(1)
    {
        char lc = keyb_get_character();
        
        if(!lc)
            continue;
        
        char str[2] = {lc, 0};
        cmd_bfr[i++] = lc;
        screen_print(str);

        if(lc == '\b' && i > 0)
            i = i - 2;

        if(lc != '\n')
            continue;

        // everything to uppercase, since this makes everything easier for us
        to_uc(cmd_bfr, strlen(cmd_bfr));

        // extra newline to allow for a visual seperation of user input and program output
        screen_print("\n");

        // exec command and set-up for next command
        if(processor_execute_command(cmd_bfr))
            print_did_not_exec_correctly(cmd_bfr);
            
        memset(cmd_bfr, COMMAND_BUFFER_SIZE, 0);
        i = 0;
        screen_print(PROMPT);
    }

    return err;
}
