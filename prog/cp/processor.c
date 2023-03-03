/*
MIT license
Copyright (c) 2022-2023 Maarten Vermeulen

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

#include "include/fileman.h"
#include "include/commands.h"
#include "include/processor.h"
#include "include/config.h"
#include "include/cp_exit_codes.h"

#define MAX_LINE_LEN        512 // bytes
#define AUTOEXEC_FILENAME   "/AUTOEXEC"

#define CHECK_COMMAND(a) !strcmp_until(a, cmd_bfr, strlen(a))

err_t g_last_error = EXIT_CODE_GLOBAL_SUCCESS;

static uint8_t processor_exec_internal_command(char *cmd_bfr, char *shadow)
{
    uint8_t did_execute = 0;

    if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_VER)))
        command_ver();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_CD)) && !fileman_contains_disk(cmd_bfr))
        command_cd(cmd_bfr);
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_PWD)))
        command_pwd();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_CLEAR)))
        command_clear();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_DIR)))
        command_dir();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_ECHO)))
        command_echo(shadow);
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_HELP)))
        command_help();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_ERRLVL)))
        command_errlvl();

    return did_execute;
}

static char *processor_ignore_leading_spaces(char *bfr)
{
    uint32_t i = 0;
    size_t len = strlen(bfr);

    for(; i < len; ++i)
        if(bfr[i] != ' ')
            break;
    
    return &bfr[i];
}

void processor_set_last_error(err_t err)
{
    g_last_error = err;
}

err_t processor_get_last_error(void)
{
    return g_last_error;
}

err_t processor_execute_command(char *cmd_bfr, char *shadow)
{
     if(!cmd_bfr)
        return EXIT_CODE_CP_NO_COMMAND;
        
    uint32_t end = find_in_str(cmd_bfr, "\n");
    cmd_bfr[end] = '\0';

    char *cmd = processor_ignore_leading_spaces(cmd_bfr);
    char *shdw = processor_ignore_leading_spaces(shadow);

    if(cmd[0] == '\0')
        return EXIT_CODE_GLOBAL_SUCCESS;

    uint8_t ran_internal = processor_exec_internal_command(cmd, shdw);

    if(ran_internal)
        return EXIT_CODE_GLOBAL_SUCCESS;
    
    err_t err = g_last_error = EXIT_CODE_GLOBAL_SUCCESS;

    char *str = valloc(MAX_PATH_LEN + 1);
    uint32_t len = 0;
    getcwd(str, &len);

    char *path = fileman_abspath_or_cwd(cmd, config_get_bin_path(), str);

    if(!path)
        return EXIT_CODE_CP_NO_COMMAND;
    
    g_last_error = err = program_start_new(path);
    vfree(str);

    return err;
}

void processor_execute_autoexec(void)
{
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    size_t ignore;

    file_t *autoexec = read_file_from_bootdisk(AUTOEXEC_FILENAME, &err, &ignore);

    if(err)
        return;
    
    char *out = valloc(MAX_LINE_LEN), *shdw = valloc(MAX_LINE_LEN);

    if(!out)
        return;

    uint32_t pindex = 0;
    while(str_get_part(out, autoexec, "\n", &pindex))
    {
        to_uc(out, strlen(out));
        processor_execute_command(out, out);
    }

    vfree(out);
    vfree(autoexec);
}

