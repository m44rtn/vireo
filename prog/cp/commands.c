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

#include "include/screen.h"
#include "include/fileman.h"
#include "include/commands.h"
#include "include/info.h"

#define MAX_INFO_STR_LEN    128
#define INFO_VER_STR_START  "CP "

#define DIR_DIRTXT_INDENT   18
#define DIR_FILESIZE_INDENT 26

static char *command_create_cp_ver_str(void)
{
    // FIXME: internal memory pool?
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
}

static void command_set_wd_bootdisk(char *path)
{
    char *bd = disk_get_bootdisk();
    char *out = valloc(MAX_PATH_LEN + 1);

    if(!out)
        return;
    
    merge_disk_id_and_path(bd, path, out);
    setcwd(out);

    vfree(bd);
    vfree(out);
}

static void command_append_to_current_wd(char *new_part)
{
    char *out = valloc(MAX_PATH_LEN + 1);
    uint32_t len = 0;

    getcwd(out, &len);
    
    merge_disk_id_and_path(out, new_part, out);
    setcwd(out);

    vfree(out);
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
    
    space_index++;
    if(cmd_bfr[space_index] == '/')
        command_set_wd_bootdisk(&cmd_bfr[space_index]);
    else if(fileman_contains_disk(&cmd_bfr[space_index]))
        setcwd(&cmd_bfr[space_index]);
    else
        command_append_to_current_wd(&cmd_bfr[space_index]);
    
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
    // TODO: add ability to get dir of working dir AND given dir
    
    uint32_t len = 0;
    char *path = valloc(MAX_PATH_LEN + 1);
    getcwd(path, &len);

    err_t err = 0;
    fs_dir_contents_t *dir = fs_dir_get_contents(path, &len, &err);

    vfree(path);

    if(err)
        return;

    uint8_t x = 0, y = 0;
    for(uint32_t i = 0; i < len; ++i)
    {
        screen_print(" ");

        screen_get_cursor_pos(screen_get_width(), &x, &y);
        screen_print(dir[i].name);
        
        screen_set_cursor_pos(DIR_DIRTXT_INDENT, y);
        if((dir[i].attrib & FAT_FILE_ATTRIB_DIR) == FAT_FILE_ATTRIB_DIR)
            { screen_print("<DIR>\n"); continue; }
        
        screen_set_cursor_pos(DIR_FILESIZE_INDENT, y);
        
        char s[24];
        str_add_val(s, "%i b", dir[i].file_size);
        screen_print(s);

        screen_print("\n");
        
    }       
}
