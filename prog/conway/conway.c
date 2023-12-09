/*
MIT license
Copyright (c) 2019-2023 Maarten Vermeulen

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

#include "conway.h"

// syslib
#include "screen.h"
#include "kernel.h"
#include "exit_code.h"
#include "util.h"
#include "scancode.h"
#include "api.h"
#include "ps2keyb.h"
#include "call.h"

#define PROGRAM_NAME "CONWAY"
#include "debug.h"

screen_info_t *screen_info = NULL;
api_space_t g_keyb_api = 0;

static void conway_fillScreen(void);
static uint8_t conway_check_neighbours(char *buffer, uint32_t x, uint32_t y);

/**
 * @brief Find keyboard driver API. Store api space in `g_keyb_api`.
 * 
 */
static void conway_get_keyb_api_space(void)
{
    api_listing_t *list = api_get_syscall_listing();

    for(uint32_t i = 0; i < 0xFF; ++i)
        if(!strcmp(list[i].filename, "PS2KEYB.DRV"))
        {
            g_keyb_api = list[i].start_syscall_space;
            break;
        };
}

/**
 * @brief Returns the last key pressed by the user
 * 
 * @return uint16_t keycode returned by PS2KEYB.DRV
 */
static uint16_t conway_keyb_get_last_key(void)
{
    ps2keyb_api_req req = 
    {
        .hdr.system_call = (syscall_t) (g_keyb_api + PS2KEYB_CALL_LAST_KEY)
    };
    PERFORM_SYSCALL(&req);

    return (uint16_t) req.hdr.response;    
}

/**
 * @brief Entry point
 * 
 * @return int exit code (always EXIT_CODE_GLOBAL_SUCCESS)
 */
int main(void)
{
    conway_get_keyb_api_space();

    screen_print("Press ESC to exit this program.\n");
    kernel_sleep(2000);

    conways_game_of_life();

    return EXIT_CODE_GLOBAL_SUCCESS;
}

/**
 * @brief Game of Life loop
 * 
 */
void conways_game_of_life(void)
{
    uint8_t x, y;
    uint8_t neighbours;

    err_t err; // ignored
    screen_info = screen_get_info(&err);

    conway_fillScreen();
    
    while(1)
    {
        kernel_sleep(64); // ~ 16 fps
        void *b_screen = screen_get_buffer(&err);

        assert(!err);
        
        for(y = 0; y < screen_info->height; ++y)
        {
            for(x = 0; x < screen_info->width; ++x)
            {
                neighbours = conway_check_neighbours(b_screen, x, y);
                char c = conway_get_screen_byte(b_screen, x, y);

                // screen_set_cursor_pos((uint8_t) x, (uint8_t) y);

                if(neighbours == 3)
                    screen_put_char_at('X', x, y);
                else if(c == 'X' && neighbours == 2)
                    screen_put_char_at('X', x, y);
                else
                    screen_put_char_at(' ', x, y);
          }
        }

        vfree(b_screen);

        if(conway_keyb_get_last_key() == KEYCODE_ESC)
            break;
    }

    screen_clear();
}

/**
 * @brief Creates initial Conway's Game of Life pattern
 * 
 */
static void conway_fillScreen(void)
{
    screen_clear();

    // XX
    // XX
    screen_put_char_at('X', 1, 5);
    screen_put_char_at('X', 2, 5);
    screen_put_char_at('X', 1, 6);
    screen_put_char_at('X', 2, 6);

    //       XX
    //      X  
    //     X    
    //     X   
    //     X   
    //      X
    //       XX
    screen_put_char_at('X', 14, 2);
    screen_put_char_at('X', 13, 2);
    screen_put_char_at('X', 12, 3);
    screen_put_char_at('X', 11, 4);
    screen_put_char_at('X', 11, 5);
    screen_put_char_at('X', 11, 6);
    screen_put_char_at('X', 12, 7);
    screen_put_char_at('X', 13, 8);
    screen_put_char_at('X', 14, 8);

    // X
    screen_put_char_at('X', 15, 5);

    // X
    //  X
    //  XX
    //  X
    // X
    screen_put_char_at('X', 16, 3);
    screen_put_char_at('X', 17, 4);
    screen_put_char_at('X', 17, 5);
    screen_put_char_at('X', 18, 5);
    screen_put_char_at('X', 17, 6);
    screen_put_char_at('X', 16, 7);

    //   X
    // XX
    // XX
    // XX
    //   X
    screen_put_char_at('X', 23, 1);

    screen_put_char_at('X', 21, 2);
    screen_put_char_at('X', 21, 3);
    screen_put_char_at('X', 21, 4);
    screen_put_char_at('X', 22, 2);
    screen_put_char_at('X', 22, 3);
    screen_put_char_at('X', 22, 4);

    screen_put_char_at('X', 23, 5);

    // X
    // X
    // 
    // 
    // 
    // X
    // X
    screen_put_char_at('X', 25, 0);
    screen_put_char_at('X', 25, 1);
    screen_put_char_at('X', 25, 5);
    screen_put_char_at('X', 25, 6);


    // XX
    // XX
    screen_put_char_at('X', 35, 2);
    screen_put_char_at('X', 36, 2);
    screen_put_char_at('X', 35, 3);
    screen_put_char_at('X', 36, 3);
}

/**
 * @brief Counts the neighbours of one character cell (i.e., how many `X` are next to this cell)
 * 
 * @param buffer Screen buffer
 * @param x Current x
 * @param y Current y
 * @return uint8_t Number of neighbours
 */
static uint8_t conway_check_neighbours(char *buffer, uint32_t x, uint32_t y)
{
    uint32_t xmax = x + 2;
    uint8_t cells = 0;

    // top & bottom
    for(uint32_t tx = (x == 0) ? x : x - 1; tx < xmax; ++tx)
    {
        if(y > 0 && conway_get_screen_byte(buffer, tx, y - 1) == 'X')
            cells++;
        
        if(conway_get_screen_byte(buffer, tx, y + 1) == 'X')
            cells++;
    }
    
    // left & right
    if(x > 0 && conway_get_screen_byte(buffer, x - 1, y) == 'X')
        cells++;
    if(conway_get_screen_byte(buffer, x + 1, y) == 'X')
        cells++;

    return cells;

}

/**
 * @brief Gets a character from the screen buffer
 * 
 * @param buffer Screen buffer
 * @param x Current x
 * @param y Current y
 * @return char Character; Expected to be either ' ' or 'X'.
 */
char conway_get_screen_byte(char *buffer, uint32_t x, uint32_t y)
{
    if(x > screen_info->width || y > screen_info->height)
        return ' ';

    return buffer[(y * screen_info->width + x) * screen_info->depth];
}
