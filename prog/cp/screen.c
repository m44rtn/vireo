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
#include "screen.h"
#include "memory.h"
#include "util.h"

#include "include/screen.h"

#define SCREEN_HEIGHT 25
#define SCREEN_WIDTH  80

/**
 * @brief Prepare the screen (cursor) for the first user prompt
 *        (i.e., the "$ ").
 * 
 */
void screen_prepare_for_first_prompt(void)
{
    err_t ignored_err;
    screen_info_t *info = screen_get_info(&ignored_err);

    uint32_t skip_lines = SCREEN_HEIGHT - 2;

    if(info)
        { skip_lines = info->height - 2; vfree(info); }

    screen_set_cursor_pos(0, (uint8_t) skip_lines);
}

/**
 * @brief Returns the height of the screen.
 * 
 * @return uint16_t Height of the screen.
 */
uint16_t screen_get_height(void)
{
    uint16_t ans = SCREEN_WIDTH;

    err_t ignored_err;
    screen_info_t *info = screen_get_info(&ignored_err);

    if(info)
        { ans = (uint16_t) info->height; vfree(info); }
    
    return ans;
}

/**
 * @brief Returns the width of the screen.
 * 
 * @return uint16_t Width of the screen.
 */
uint16_t screen_get_width(void)
{
    uint16_t ans = SCREEN_WIDTH;

    err_t ignored_err;
    screen_info_t *info = screen_get_info(&ignored_err);

    if(info)
        { ans = (uint16_t) info->width; vfree(info); }
    
    return ans;
}

/**
 * @brief Prints "no command or filename".
 * 
 * @param cmd_bfr Command buffer.
 */
void screen_print_no_command(char *cmd_bfr)
{
    char *str = valloc(strlen(cmd_bfr));

    if(str)
    {
        str_add_val(str, "%s: no command or filename\n", (uint32_t) cmd_bfr);
        screen_print(str);
        vfree(str);
    }
    else
        screen_print("No command or filename\n");
}
