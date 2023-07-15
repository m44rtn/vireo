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
#include "kernel.h"
#include "util.h"
#include "screen.h"
#include "memory.h"
#include "disk.h"
#include "fs.h"
#include "scancode.h"

#include "include/screen.h"
#include "include/fileman.h"
#include "include/commands.h"
#include "include/info.h"
#include "include/processor.h"
#include "include/keyb.h"

#define MAX_INFO_STR_LEN    128
#define INFO_VER_STR_START  "CP "

#define DIR_DIRTXT_INDENT   18
#define DIR_FILESIZE_INDENT 26

#define HELP_TXT_INDENT     12

typedef enum 
{
    ALL_OK,
    QUIT,
    CONTINUE
} more_t;

static more_t command_more(uint16_t scr_width, uint16_t scr_height)
{
    uint8_t x = 0, y = 0;
    screen_get_cursor_pos(scr_width, &x, &y);

    if(y < (scr_height - 3))
        return ALL_OK; // everything ok

    screen_print("-- press [C] to continue, or [Q] to quit --");
    
    uint16_t c = 0;
    while(c != 'Q' && c != 'C')
        c = (uint16_t) (keyb_get_last_pressed());
    
    screen_print("\n");
    
    if(c == 'Q')
        return QUIT;
    
    return CONTINUE;
}

char *command_create_cp_ver_str(void)
{
    char *str = valloc(MAX_INFO_STR_LEN);
    
    if(!str)
        return NULL;

    str_add_val(str, INFO_VER_STR_START "v%i.", MAJOR);
    uint32_t i = strlen(str);

    str_add_val(&str[i], "%i", MINOR);
    i = strlen(str);

    str_add_val(&str[i], "%s", (uint32_t) REV);

    str_add_val(&str[i], " (build %i)", BUILD);
    
    return str;
}

void command_ver(void)
{
    char *kernel_ver = kernel_get_version_str();
    char *cp_ver = command_create_cp_ver_str();

    screen_print("Kernel: ");
    screen_print(kernel_ver);
    screen_print("\n");
    vfree(kernel_ver);

    screen_print("CP: ");
    screen_print(cp_ver);
    screen_print("\n");
    vfree(cp_ver);

    screen_print("\nCopyright (c) 2019-2023 Maarten Vermeulen\n");
}

static err_t command_set_wd_bootdisk(char *path)
{
    char *bd = disk_get_bootdisk();
    char *out = valloc(MAX_PATH_LEN + 1);

    if(!out)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
    
    merge_disk_id_and_path(bd, path, out);
    err_t err = setcwd(out);

    vfree(bd);
    vfree(out);

    return err;
}

static err_t command_append_to_current_wd(char *new_part)
{
    char *out = valloc(MAX_PATH_LEN + 1);

    if(!out)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;

    uint32_t len = 0;

    getcwd(out, &len);
    
    merge_disk_id_and_path(out, new_part, out);
    err_t err = setcwd(out);

    vfree(out);
    return err;
}

void command_cd(char *cmd_bfr)
{
    uint32_t space_index = find_in_str(cmd_bfr, " ");

    cmd_bfr[find_in_str(cmd_bfr, "\n")] = '\0';

    uint32_t i = space_index;
    while((i = find_in_str(&cmd_bfr[space_index + 1], " ")) != MAX)
        space_index = space_index + i + 1;

    if(space_index == MAX || cmd_bfr[space_index] == '\0')
        { command_pwd(); return; }
    
    err_t err = 0;

    space_index++;
    if(cmd_bfr[space_index] == '/')
        err = command_set_wd_bootdisk(&cmd_bfr[space_index]);
    else if(fileman_contains_disk(&cmd_bfr[space_index]))
        err = setcwd(&cmd_bfr[space_index]);
    else
        err = command_append_to_current_wd(&cmd_bfr[space_index]);

    if(!err)
        return;

    char *s = valloc(MAX_PATH_LEN + 64); 
    if(!s)
        return;
    str_add_val(s, "%s: not a valid directory\n", (uint32_t) (&cmd_bfr[space_index]));
    screen_print(s);
    vfree(s);    
}

void command_pwd(void)
{
    char str[255]; 
    uint32_t len = 0;

    getcwd(str, &len); 
    
    screen_print(str);
    screen_print("\n");
}

void command_clear(void)
{
    screen_clear();
    screen_prepare_for_first_prompt();
}

void command_dir(void)
{    
    uint32_t len = 0;
    char *path = valloc(MAX_PATH_LEN + 1);
    getcwd(path, &len);

    err_t err = 0;
    fs_dir_contents_t *dir = fs_dir_get_contents(path, &len, &err);

    vfree(path);

    if(err)
        return;

    uint16_t scr_width = screen_get_width();
    uint16_t scr_height = screen_get_height();

    uint8_t x = 0, y = 0;
    size_t total_size = 0;
    char s[24];
    
    for(uint32_t i = 0; i < len; ++i)
    {
        screen_print(" ");

        screen_get_cursor_pos(screen_get_width(), &x, &y);
        screen_print(dir[i].name);       
        
        screen_set_cursor_pos(DIR_DIRTXT_INDENT, y);
        if((dir[i].attrib & FAT_FILE_ATTRIB_DIR) == FAT_FILE_ATTRIB_DIR)
            screen_print("<DIR>");
        else 
        {
            screen_set_cursor_pos(DIR_FILESIZE_INDENT, y);
            
            total_size += dir[i].file_size;
            str_add_val(s, "%i b", dir[i].file_size);
            screen_print(s);
        }

        screen_print("\n");

        if(len < (scr_height - 1u))
            continue;

        more_t next_action = command_more(scr_width, scr_height);
        
        if(next_action == QUIT)
            break;
        else if(next_action == ALL_OK)
            continue;
                
        screen_clear();
        screen_print("\n");
    }  

    screen_print("\n");

    screen_get_cursor_pos(screen_get_width(), &x, &y);
    screen_set_cursor_pos(DIR_DIRTXT_INDENT, y);
    str_add_val(s, "%i items\n", len);
    screen_print(s);

    screen_get_cursor_pos(screen_get_width(), &x, &y);
    screen_set_cursor_pos(DIR_DIRTXT_INDENT, y);
    str_add_val(s, "%i ", total_size);
    screen_print(s);
    screen_print("bytes total\n");

    vfree(dir);   
}

void command_echo(char *cmd_bfr)
{
    uint32_t start = strlen("ECHO ");
    screen_print(&cmd_bfr[start]); // \n already there due to how the command buffer works
}

static void help_print_command(const char *command, const char *helptxt)
{
    uint8_t x, y;
    screen_print(command);
    screen_get_cursor_pos(screen_get_width(), &x, &y);
    screen_set_cursor_pos(HELP_TXT_INDENT, y);
    screen_print(helptxt);
}

void command_help(void)
{
    help_print_command(INTERNAL_COMMAND_CD " [PATH]", "navigate to [PATH], executes PWD if no path was given\n");
    help_print_command(INTERNAL_COMMAND_PWD, "prints current working directory\n");
    help_print_command(INTERNAL_COMMAND_CLEAR, "clears the screen\n");
    help_print_command(INTERNAL_COMMAND_DIR, "shows contents of current working directory\n");
    help_print_command(INTERNAL_COMMAND_ECHO " [TXT]", "prints [TXT] to the screen\n");
    help_print_command(INTERNAL_COMMAND_HELP, "shows this help message\n");
    help_print_command(INTERNAL_COMMAND_VER, "prints copyright and version of CP and the kernel\n");
    help_print_command(INTERNAL_COMMAND_ERRLVL, "prints exit code of last ran binary\n");
    help_print_command(INTERNAL_COMMAND_TYPE " [PATH]", "prints the contents of file at [PATH]\n");
}

void command_errlvl(void)
{
    char s[12];
    str_add_val(s, " 0x%x\n", (uint32_t)processor_get_last_error());
    screen_print(s);
}

void command_type(char *cmd_bfr)
{
    // check where the file is located (cwd or given path)
    char *str = valloc(MAX_PATH_LEN + 1);
    uint32_t len = 0;
    getcwd(str, &len);

    uint32_t i = 1;
    str_get_part(cmd_bfr, cmd_bfr, " ", &i);
    char *path = fileman_abspath_or_cwd(cmd_bfr, str, str);

    if(!path)
        { vfree(str); screen_print("File not found - "); screen_print(cmd_bfr); screen_print("\n"); return; }

    vfree(str);
    
    // read the file
    size_t s = 0;
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    char *file_contents = fs_read_file(path, &s, &err);
    vfree(path);

    if(!file_contents || err)
        { screen_print("Error reading file\n"); return; }

    char *out = valloc(s + 1);
    uint32_t index = 0;

    uint16_t scr_width = screen_get_width();
    uint16_t scr_height = screen_get_height();
    
    // print the file contents
    while(str_get_part(out, file_contents, "\n", &index))
    { 
        screen_print(out); 
        screen_print("\n"); 

        more_t next_action = command_more(scr_width, scr_height);
        
        if(next_action == QUIT)
            break;
        else if(next_action == ALL_OK)
            continue;
                
        screen_clear();
        screen_print("\n");
    }

    vfree(out);
}
