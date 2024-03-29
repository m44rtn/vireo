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

#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include "types.h"

#define PROCESSOR_ENV_VAR_DELIM         "$"
#define PROCESSOR_MAX_ENV_VAR_NAME_LEN  32
#define PROCESSOR_MAX_ENV_VAR_VALUE_LEN 96
#define PROCESSOR_MAX_ENV_VARS          32
#define PROCESSOR_ENV_VAR_SPACE_SIZE    (PROCESSOR_MAX_ENV_VARS * (PROCESSOR_MAX_ENV_VAR_NAME_LEN + \
                                        PROCESSOR_MAX_ENV_VAR_VALUE_LEN))

/**
 * @brief Stores the name and value of an environment variable
 * 
 */
typedef struct processor_envvar_t
{
    char name[PROCESSOR_MAX_ENV_VAR_NAME_LEN];
    char value[PROCESSOR_MAX_ENV_VAR_VALUE_LEN];
} __attribute__((packed)) processor_envvar_t;

err_t processor_init(void);
void processor_replace_with_environment_variables(char *cmd_bfr, char *shdw);

char *processor_ignore_leading_spaces(char *bfr);
void processor_set_last_error(err_t err);
err_t processor_get_last_error(void);
err_t processor_execute_command(char *cmd_bfr, char *shadow);
err_t processor_execute_cp_script(file_t *file);
err_t processor_execute_autoexec(void);

err_t processor_set_environment_variable(const char *name, const char *value);
err_t processor_unset_environment_variable(const char *name);
err_t processor_get_environment_variable_value_by_name(const char *name, char *o_value, uint32_t *o_id);
err_t processor_get_environment_variable_value_by_id(uint32_t id, char *o_name, char *o_value);

#endif // __PROCESSOR_H__
