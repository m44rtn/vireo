/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

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

#include "../include/types.h"

#include "../screen/screen_basic.h"

#include "../util/util.h"

static void conway_fillScreen(void);
static uint8_t conway_check_neighbours(uint32_t x, uint32_t y);
static void conway_shouldIBeAliveOrNotAndMakeItSo(uint32_t x, uint32_t y);

void conways_game_of_life(void)
{
    uint32_t x, y;
    uint8_t neighbours;

    screen_basic_clear_screen();

    conway_fillScreen();
    
    while(1)
    {
        sleep(200);
        for(y = 0; y < 25; ++y)
        {
            for(x = 0; x < 80; ++x)
            {
                if(screen_basic_getchar(x, y) != 'X')
                {
                    conway_shouldIBeAliveOrNotAndMakeItSo(x, y);
                    continue;
                }

                neighbours = conway_check_neighbours(x, y);

                if(neighbours < 2 || neighbours > 3) 
                    screen_basic_putchar(x, y, ' '); /* you're dead mate */
                
          }
        }

        
    }
}

static void conway_fillScreen(void)
{
    uint32_t x, y;

    for(y = 0; y < 25; ++y)
            for(x = 0; x < 80; ++x)
                if((!y || x % 2 || y % 3) || (x % 5 && y % 7)) screen_basic_putchar(x, y, 'X'); /* change this! */
}

static uint8_t conway_check_neighbours(uint32_t x, uint32_t y)
{
    uint32_t xa;
    uint8_t live_cells = 0;

    uint8_t isxScreenWidth =  !((x+1) == SCREEN_BASIC_WIDTH);

    /* check the three places above us (if there's a row above us) */
    if(y > 0)
        for(xa = (x <= 0) ? 0 : x-1; xa <= (x + isxScreenWidth); ++xa)
            if(screen_basic_getchar(xa, y-1) == 'X') ++live_cells;
    
    /*trace("live_cells %i\n", live_cells);*/
    
    /* check our row */
    if(x > 0)
        if(screen_basic_getchar(x-1, y) == 'X') ++live_cells;
    
    if(x+1 <= SCREEN_BASIC_WIDTH)
        if(screen_basic_getchar(x+1, y) == 'X') ++live_cells;
    
    
    /* check row below us if there is one */
    if(y < SCREEN_BASIC_HEIGHT)
        for(xa = (x <= 0) ? 0 : x-1; xa <= (x + isxScreenWidth); ++xa)
            if(screen_basic_getchar(xa, y+1) == 'X') ++live_cells;

    return live_cells;

}

static void conway_shouldIBeAliveOrNotAndMakeItSo(uint32_t x, uint32_t y)
{
    if(conway_check_neighbours(x, y) == 3)
        screen_basic_putchar(x, y, 'X');
}