/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

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

#ifndef __SCREEN_BASIC_H__
#define __SCREEN_BASIC_H__

/* screen dimensions */
#define SCREEN_BASIC_WIDTH          (unsigned char) 80
#define SCREEN_BASIC_HEIGHT         (unsigned char) 25
#define SCREEN_BASIC_DEPTH          (unsigned char) 2

/* EXIT CODES */
#define SCREEN_BASIC_EXIT_CODE_CURSOR_MOVE_FAIL     16

/* SCREEN DATA FLAGS */
#define SCREEN_BASIC_CURSOR_ENABLED                   1

/* amount of hexadecimal digits in trace */
#define SCREEN_BASIC_HEX_DIGITS_USE_DEFAULT         0

unsigned char screen_basic_init(void);

void screen_basic_api(void *req);

void screen_basic_enable_cursor(unsigned char cursor_start, unsigned char cursor_end);
void screen_basic_disable_cursor(void);
unsigned char screen_basic_move_cursor(unsigned short x, unsigned short y);
unsigned short screen_basic_get_cursor_position(void);

void screen_basic_set_screen_color(unsigned char color);
void screen_set_hexdigits(unsigned char value);

void print(const char* text);
void print_value(const char* text, unsigned int val);
void print_at(const char *str, unsigned int x, unsigned int y);
void screen_basic_clear_screen(void);

char screen_basic_getchar(unsigned int x, unsigned int y);
void screen_basic_putchar(unsigned int x, unsigned int y, char c);


#endif
