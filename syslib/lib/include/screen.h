/*
MIT license
Copyright (c) 2021 Maarten Vermeulen

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

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "types.h"
#include "call.h"

#define SCREEN_COLOR_DEFAULT        0x07

#define SCREEN_COLOR_BLACK          0x00
#define SCREEN_COLOR_BLUE           0x01
#define SCREEN_COLOR_GREEN          0x02
#define SCREEN_COLOR_CYAN           0x03
#define SCREEN_COLOR_RED            0x04
#define SCREEN_COLOR_MAGENTA        0x05
#define SCREEN_COLOR_BROWN          0x06
#define SCREEN_COLOR_LIGHT_GRAY     0x07
#define SCREEN_COLOR_DARK_GRAY      0x08
#define SCREEN_COLOR_LIGHT_BLUE     0x09
#define SCREEN_COLOR_LIGHT_GREEN    0x0a
#define SCREEN_COLOR_LIGHT_CYAN     0x0b
#define SCREEN_COLOR_LIGHT_RED      0x0c
#define SCREEN_COLOR_LIGHT_MAGENTA  0x0d
#define SCREEN_COLOR_YELLOW         0x0e
#define SCREEN_COLOR_WHITE          0x0f

typedef enum screen_mode_t
{
    VGA_MODE_3      // aka BIOS/'default' text mode
} screen_mode_t;

typedef struct screen_info_t
{
    screen_mode_t mode;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} __attribute__((packed)) screen_info_t;

typedef uint32_t color_t;

// returns information about the screen
screen_info_t *screen_get_info(err_t *err);

// prints a null-terminated string to the screen
err_t screen_print(const char *_str);

// prints a null-terminated string to the screen at the specified location
err_t screen_print_at(const char *_str, uint32_t _x, uint32_t _y);

// returns a copy of the current screen buffer
char *screen_get_buffer(err_t *err);

// returns byte/char from the screen buffer at the specified location
char screen_get_byte_at(uint32_t _x, uint32_t _y, err_t *err);

// sets the background/foreground color to the specified color
err_t screen_set_color(color_t _color);

// clears the entire screen
void screen_clear(void);

#endif // __SCREEM_H__
