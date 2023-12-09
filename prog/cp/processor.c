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

#define GET_SMALLEST(a, b)  ((a < b) ? a : b)

/**
 * @brief Checks whether input by a user is a specific internal command
 * 
 */
#define CHECK_COMMAND(command) ((!strcmp_until(command, cmd_bfr, strlen(command))) && \
                                 ((cmd_bfr[strlen(command)] == ' ') || (cmd_bfr[strlen(command)] == '\n') \
                               || (cmd_bfr[strlen(command)] == '\0')))

err_t g_last_error = EXIT_CODE_GLOBAL_SUCCESS;
processor_envvar_t *g_env_vars = NULL;

/**
 * @brief Initializes the processor (currently allocates space for
 *          environment variables)
 * 
 * @return err_t Exit code
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success
 *                  - EXIT_CODE_GLOBAL_OUT_OF_MEMORY unable to allocate memory
 */
err_t processor_init(void)
{
    g_env_vars = valloc(PROCESSOR_ENV_VAR_SPACE_SIZE);

    if(!g_env_vars)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;

    memset(g_env_vars, PROCESSOR_ENV_VAR_SPACE_SIZE, 0);
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

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
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_SET)))
        *o_err = command_set(cmd_bfr, shadow);
    else if((did_execute = CHECK_COMMAND(INTERNAL_COMMAND_UNSET)))
        *o_err = command_unset(cmd_bfr);
    
    return did_execute;
}

/**
 * @brief returns a pointer ignoring the leading spaces in a buffer
 * 
 * @param bfr pointer to a string
 * @return char* points after the leading spaces
 */
char *processor_ignore_leading_spaces(char *bfr)
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
    shadow[end] = '\0';

    char *cmd = processor_ignore_leading_spaces(cmd_bfr);
    char *shdw = processor_ignore_leading_spaces(shadow);

    if(cmd[0] == '\0')
        return EXIT_CODE_GLOBAL_SUCCESS;
    
    processor_replace_with_environment_variables(cmd_bfr, shadow);

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
 * @brief Replaces "$name;" of an environment variable with its value, helper for processor_replace_with_environment_variables()
 * 
 * @param cmd_bfr Buffer to replace in
 * @param value Environment variable value
 * @param end_of_variable_name End of the name of the variable ('$' character included)
 */
static void processor_do_replace_action(char *cmd_bfr, char *value, uint32_t end_of_variable_name)
{
    size_t value_len = strlen(value);
    char *tmp = valloc(4096);

    if(!tmp)
    {
        screen_print("<CP> Out of memory\n");
        return;
    }

    memcpy(tmp, &cmd_bfr[end_of_variable_name + 1], strlen(&cmd_bfr[end_of_variable_name + 1]) + 1);

    memcpy(cmd_bfr, value, value_len);
    memcpy(&cmd_bfr[value_len], tmp, strlen(tmp) + 1);

    vfree(tmp);
}

/**
 * @brief Replaces the environment variable names in cmd_bfr and shdw with their values
 *        if there are any
 * 
 * @param cmd_bfr User input
 * @param shdw User input
 */
void processor_replace_with_environment_variables(char *cmd_bfr, char *shdw)
{
    uint32_t ignored_value = 0;
    uint32_t start_of_var = 0;
    uint32_t end_of_var = 0;
    uint32_t index = 0;

    while((index = find_in_str(&cmd_bfr[start_of_var], PROCESSOR_ENV_VAR_DELIM)) != MAX)
    {
        start_of_var = start_of_var + index;
        end_of_var = find_in_str(&cmd_bfr[start_of_var], ";");

        if(end_of_var == MAX)
            return; // INVALID

        char value[PROCESSOR_MAX_ENV_VAR_VALUE_LEN + 1];

        // Using `value` here as input (as name) to the processor_get_environment_variable_value_by_name() function as well
        memcpy(value, &cmd_bfr[start_of_var + 1], end_of_var - 1);
        value[end_of_var - 1] = '\0';
        to_uc(value, end_of_var - 1);

        err_t err = processor_get_environment_variable_value_by_name(value, value, &ignored_value);
        value[PROCESSOR_MAX_ENV_VAR_VALUE_LEN] = '\0';

        if(err == EXIT_CODE_GLOBAL_GENERAL_FAIL)
        {
            screen_set_color((SCREEN_COLOR_BLACK << 4) | SCREEN_COLOR_YELLOW);
            screen_print("<CP> Warning: environment variable does not exist.\n\n");
            screen_set_color(SCREEN_COLOR_DEFAULT);

            // To avoid find_in_str() finding the same '$' over and over, causing an infinite loop
            start_of_var += 1;

            continue;
        }

        processor_do_replace_action(&cmd_bfr[start_of_var], value, end_of_var);
        processor_do_replace_action(&shdw[start_of_var], value, end_of_var);

        // To avoid find_in_str() finding the same '$' over and over, causing an infinite loop
        start_of_var += 1;
    }
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

/**
 * @brief Executes all commands within the autoexec file
 * 
 * @return err_t Error code:
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success.
 *                  - Any error from filesystem driver.
 *                  - Any error from processor_execute_cp_script().
 */
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

/**
 * @brief Sets an environment variable within CP.
 *        NOTE: will truncate name or value when its size is too large.
 * 
 * @param name Name of new environment variable
 * @param value Value of new environment variable
 * @return err_t Exit code:
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success
 *                  - EXIT_CODE_GLOBAL_GENERAL_FAIL when the environment variable could not be added
 *                  - EXIT_CODE_GLOBAL_RESERVED when an environment variable with the same name already
 *                    exists
 *                  - EXIT_CODE_GLOBAL_INVALID on invalid environment variable name/value
 */
err_t processor_set_environment_variable(const char *name, const char *value)
{
    if(strlen(name) == 0)
        return EXIT_CODE_GLOBAL_INVALID;

    for(uint32_t i = 0; i < PROCESSOR_MAX_ENV_VARS; ++i)
    {
        if(!strcmp(&g_env_vars[i].name[0], name))
            return EXIT_CODE_GLOBAL_RESERVED;

        if(g_env_vars[i].name[0] != 0)
            continue;
        
        // Found an empty entry
        memcpy(&g_env_vars[i].name[0], name, GET_SMALLEST(strlen(name) + 1, PROCESSOR_MAX_ENV_VAR_NAME_LEN));
        memcpy(&g_env_vars[i].value[0], value, GET_SMALLEST(strlen(value) + 1, PROCESSOR_MAX_ENV_VAR_VALUE_LEN));
        return EXIT_CODE_GLOBAL_SUCCESS;
    }

    return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

/**
 * @brief Removes an environment variable from CP
 * 
 * @param name Name of the environment variable
 * @return err_t Exit code:
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success
 */
err_t processor_unset_environment_variable(const char *name)
{    
    for(uint32_t i = 0; i < PROCESSOR_MAX_ENV_VARS; ++i)
        if(!strcmp(name, g_env_vars[i].name))
            memset(&g_env_vars[i].name[0], PROCESSOR_MAX_ENV_VAR_NAME_LEN, 0);
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

/**
 * @brief Returns value and id of environment variable called `name`
 * 
 * @param name Name of the environment variable
 * @param o_value [out] Value of environment variable
 * @param o_id [out] ID of environment variable
 * @return err_t Exit code
 *                  - EXIT_CODE_GLOBAL_SUCCESS when found
 *                  - EXIT_CODE_GLOBAL_GENERAL_FAIL when not found
 */
err_t processor_get_environment_variable_value_by_name(const char *name, char *o_value, uint32_t *o_id)
{
    for(uint32_t i = 0; i < PROCESSOR_MAX_ENV_VARS; ++i)
    {
        if(strcmp(&g_env_vars[i].name[0], name))
            continue;

        // Found the environment variable
        memcpy(o_value, g_env_vars[i].value, PROCESSOR_MAX_ENV_VAR_VALUE_LEN);
        *o_id = i;
        return EXIT_CODE_GLOBAL_SUCCESS;
    }

    return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

/**
 * @brief Returns the name and value of environment variable with ID `id`
 * 
 * @param id ID of environment variable
 * @param o_name [out] name of environment variable
 * @param o_value [out] name of environment variable
 * @return err_t Exit code
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success
 *                  - EXIT_CODE_GLOBAL_OUT_OF_RANGE on ID too large
 *                  - EXIT_CODE_GLOBAL_GENERAL_FAIL when the environment variable with ID `id`
 *                    is not initialized (not set, no name available for this id)
 */
err_t processor_get_environment_variable_value_by_id(uint32_t id, char *o_name, char *o_value)
{
    if(id >= PROCESSOR_MAX_ENV_VARS)
        return EXIT_CODE_GLOBAL_OUT_OF_RANGE;
    
    if(g_env_vars[id].name[0] == 0)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    
    memcpy(o_name, g_env_vars[id].name, PROCESSOR_MAX_ENV_VAR_NAME_LEN);
    memcpy(o_value, g_env_vars[id].value, PROCESSOR_MAX_ENV_VAR_VALUE_LEN);

    return EXIT_CODE_GLOBAL_SUCCESS;
}
