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

#define INTERNAL_COMMAND_CD     "CD"
#define INTERNAL_COMMAND_PWD    "PWD"
#define INTERNAL_COMMAND_CLEAR  "CLEAR"
#define INTERNAL_COMMAND_DIR    "DIR"
#define INTERNAL_COMMAND_ECHO   "ECHO"
#define INTERNAL_COMMAND_HELP   "HELP"
#define INTERNAL_COMMAND_VER    "VER"

void command_ver(void);
void command_cd(char *cmd_bfr);
void command_pwd(void);
void command_clear(void);
void command_dir(void);
void command_help(void);

#endif // __COMMANDS_H__