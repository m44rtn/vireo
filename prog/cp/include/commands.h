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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "types.h"

#define INTERNAL_COMMAND_CD         "CD"
#define INTERNAL_COMMAND_PWD        "PWD"
#define INTERNAL_COMMAND_CLEAR      "CLEAR"
#define INTERNAL_COMMAND_DIR        "DIR"
#define INTERNAL_COMMAND_ECHO       "ECHO"
#define INTERNAL_COMMAND_HELP       "HELP"
#define INTERNAL_COMMAND_VER        "VER"
#define INTERNAL_COMMAND_ERRLVL     "ERRLVL"
#define INTERNAL_COMMAND_TYPE       "TYPE"
#define INTERNAL_COMMAND_PAUSE      "PAUSE"
#define INTERNAL_COMMAND_SET        "SET"
#define INTERNAL_COMMAND_UNSET      "UNSET"
#define INTERNAL_COMMAND_DOTSLASH   "./"

char *command_create_cp_ver_str(void);

err_t command_ver(void);
err_t command_cd(char *cmd_bfr);
err_t command_pwd(void);
err_t command_clear(void);
err_t command_dir(void);
err_t command_echo(char *cmd_bfr);
err_t command_help(void);
err_t command_errlvl(void);
err_t command_type(char *cmd_bfr);
err_t command_pause(void);
err_t command_dotslash(char *cmd_bfr);
err_t command_set(char *cmd_bfr, char *shdw_bfr);
err_t command_unset(char *cmd_bfr);

#endif // __COMMANDS_H__