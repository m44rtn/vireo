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
#include "screen.h"

#include "include/keyb.h"
#include "include/fileman.h"
#include "include/commands.h"
#include "include/processor.h"
#include "include/config.h"
#include "include/cp_exit_codes.h"
#include "include/screen.h"

#define MAX_LINE_LEN        512 // bytes
#define AUTOEXEC_FILENAME   "/AUTOEXEC"

#define CHECK_COMMAND(a) !strcmp_until(a, cmd_bfr, strlen(a))

err_t g_last_error = EXIT_CODE_GLOBAL_SUCCESS;

/**
 * @brief Executes an internal CP command
 * 
 * @param cmd_bfr pointer to the buffer containing the user's input (Uppercase)
 * @param shadow pointer to the buffer containing the original user input (used for ECHO)
 * @param o_err output error code
 * @return uint8_t = 1 if an internal command was executed
 */
static uint8_t processor_exec_internal_command(char *cmd_bfr, char *shadow, err_t *o_err)
{
    uint8_t did_execute = 0;

    if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_VER)))
        *o_err = command_ver();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_CD)) && !fileman_contains_disk(cmd_bfr))
        *o_err = command_cd(cmd_bfr);
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_PWD)))
        *o_err = command_pwd();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_CLEAR)))
        *o_err = command_clear();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_DIR)))
        *o_err = command_dir();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_ECHO)))
        *o_err = command_echo(shadow);
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_HELP)))
        *o_err = command_help();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_ERRLVL)))
        *o_err = command_errlvl();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_TYPE)))
        *o_err = command_type(cmd_bfr);
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_PAUSE)))
        *o_err = command_pause();
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_DOTSLASH)))
        *o_err = command_dotslash(cmd_bfr);

    return did_execute;
}

/**
 * @brief returns a pointer ignoring the leading spaces in a buffer
 * 
 * @param bfr pointer to a string
 * @return char* points after the leading spaces
 */
static char *processor_ignore_leading_spaces(char *bfr)
{
    uint32_t i = 0;
    size_t len = strlen(bfr);

    for(; i < len; ++i)
        if(bfr[i] != ' ')
            break;
    
    return &bfr[i];
}

/**
 * @brief sets g_last_error to err
 * 
 * @param err 
 */
void processor_set_last_error(err_t err)
{
    g_last_error = err;
}

/**
 * @brief Returns last error stored in g_last_error
 * 
 * @return err_t last error saved in g_last_error
 */
err_t processor_get_last_error(void)
{
    return g_last_error;
}

/**
 * @brief Parse the cmd_bfr and execute the command typed
 * 
 * @param cmd_bfr user input (uppercase)
 * @param shadow original user input
 * @return err_t:
 *          - EXIT_CODE_GLOBAL_SUCCESS, if a command was executed.
 *          - EXIT_CODE_CP_NO_COMMAND, when the user input was empty, NULL or did not 
 *            contain a valid command.
 *          - Any other error passed by a command that has been executed (external programs).
 */
err_t processor_execute_command(char *cmd_bfr, char *shadow)
{
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;

     if(!cmd_bfr)
        return EXIT_CODE_CP_NO_COMMAND;
        
    uint32_t end = find_in_str(cmd_bfr, "\n");
    cmd_bfr[end] = '\0';

    char *cmd = processor_ignore_leading_spaces(cmd_bfr);
    char *shdw = processor_ignore_leading_spaces(shadow);

    if(cmd[0] == '\0')
        return EXIT_CODE_GLOBAL_SUCCESS;

    uint8_t ran_internal = processor_exec_internal_command(cmd, shdw, &err);
    g_last_error = err;

    if(ran_internal)
        return err;

    char *str = valloc(MAX_PATH_LEN + 1);
    uint32_t len = 0;
    getcwd(str, &len);

    char *path = fileman_abspath_or_cwd(cmd, config_get_bin_path(), str);

    if(!path)
    {
        g_last_error = EXIT_CODE_CP_NO_COMMAND;
        return EXIT_CODE_CP_NO_COMMAND;
    }
    
    g_last_error = err = program_start_new(path);

    // the user may have used the keyboard, which means our buffer now contains
    // the same data which we cannot use.
    keyb_empty_buffer();

    vfree(str);
    vfree(path);

    return err;
}

/**
 * @brief Executes a CP command script (i.e., a text file
 *        containing commands)
 *        NOTE: all text in the file is converted to uppercase,
 *              which results in ECHO not showing the orignal
 *              capitalization from the *actual* contents of the file.
 * 
 * @param file Pointer to file buffer.
 * @return err_t Error code:
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success.
 *                  - EXIT_CODE_GLOBAL_OUT_OF_MEMORY when out of memory.
 *                  - The last error
 */
err_t processor_execute_cp_script(file_t *file)
{
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    
    char *out = valloc(MAX_LINE_LEN);

    if(!out)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;

    uint32_t pindex = 0;
    while(str_get_part(out, file, "\n", &pindex))
    {
        to_uc(out, strlen(out));
        err_t e = processor_execute_command(out, out);

        if(e == EXIT_CODE_CP_NO_COMMAND)
        {
            screen_set_color(SCREEN_COLOR_BLACK | SCREEN_COLOR_YELLOW);
            screen_print("<CP> ");
            screen_set_color(SCREEN_COLOR_DEFAULT);
            screen_print_no_command(out);
        }
    }

    vfree(out);
    
    return err;
}
err_t processor_execute_autoexec(void)
{
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    size_t ignore;

    file_t *autoexec = read_file_from_bootdisk(AUTOEXEC_FILENAME, &err, &ignore);

    if(err)
        return err;
    
    err = processor_execute_cp_script(autoexec);

    vfree(autoexec);

    return err;
}

