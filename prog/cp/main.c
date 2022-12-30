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
#include "disk.h"

#include "include/fileman.h"
#include "include/config.h"
#include "include/keyb.h"
#include "include/processor.h"

#define PROMPT  "$ "
#define COMMAND_BUFFER_SIZE 512 // chars

static void print_did_not_exec_correctly(char *cmd_bfr)
{
    char *str = valloc(strlen(cmd_bfr));

    str_add_val(str, "%s: no command or filename, or program returned with error.\n", cmd_bfr);
    screen_print(str);

    vfree(str);
}

static uint32_t append_shadow_to_main(char *shadow, uint32_t sh_len, char *main, uint32_t main_st)
{
    uint32_t i = main_st;
    uint32_t shi = 0;

    while(shi < sh_len)
    {
        if(i > 0 && shadow[shi] == '\b')
            { shi++; i--; continue; }
        else if(i == 0 && shadow[shi] == '\b')
            { shi++; continue; }
        else if(i >= COMMAND_BUFFER_SIZE - 1 && shadow[shi] != '\n')
            { shi++; continue; }

        
        main[i] = shadow[shi];
        i++; shi++;
    }
 
    return i;
}

static void screen_magic(char *str, uint32_t n, uint32_t cdm_bfr_index)
{
    // print everything, unless we backspace when there's nothing in the command buffer
    for(uint32_t i = 0; i < n; i++)
    {
        if(cdm_bfr_index == 0 && str[i] == '\b')
            continue;
        
        char st[2] = {str[i], 0};
        screen_print(st);
        cdm_bfr_index++;
    }
}

static void set_first_working_dir(void)
{
    char *bd = disk_get_bootdisk();
    setcwd(bd);

    vfree(bd);
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
    
    set_first_working_dir();
    
    char *cmd_bfr = valloc(COMMAND_BUFFER_SIZE * 2);
    char *cmd_shadow = cmd_bfr + COMMAND_BUFFER_SIZE;

    uint32_t i = 0;

    screen_print(PROMPT);

    while(1)
    {
        uint32_t n = keyb_get_character(cmd_shadow);
        
        if(!n)
            continue;
        
        if(i + n < COMMAND_BUFFER_SIZE)
            screen_magic(cmd_shadow, n, i);       
        
        i = append_shadow_to_main(cmd_shadow, n, cmd_bfr, i);
       

        if(cmd_bfr[i - 1] != '\n')
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
