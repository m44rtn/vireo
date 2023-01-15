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

#include "../include/screen.h"

typedef struct screen_t
{
    syscall_hdr_t hdr;
    const char *str;
    uint32_t x;
    uint32_t y;
} __attribute__((packed)) screen_t;


typedef struct screen_color_t
{
    syscall_hdr_t hdr;
    color_t color;
} __attribute__((packed)) screen_color_t;

screen_info_t *screen_get_info(err_t *err)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_SCREEN_INFO};
    PERFORM_SYSCALL(&hdr);

    *err = hdr.exit_code;

    return (screen_info_t *) hdr.response_ptr;
}

err_t screen_print(const char *_str)
{
    screen_t req = {
        .hdr.system_call = SYSCALL_PRINT,
        .str = _str
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}

err_t screen_print_at(const char *_str, uint32_t _x, uint32_t _y)
{
    screen_t req = {
        .hdr.system_call = SYSCALL_PRINT_AT,
        .str = _str,
        .x = _x,
        .y = _y
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}

char *screen_get_buffer(err_t *err)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_SCREEN_BUFFER};
    PERFORM_SYSCALL(&hdr);

    *err = hdr.exit_code;

    return hdr.response_ptr;
}

char screen_get_byte_at(uint32_t _x, uint32_t _y, err_t *err)
{
    screen_t req = {
        .hdr.system_call = SYSCALL_GET_SCREEN_GET_BYTE,
        .x = _x,
        .y = _y
    };
    PERFORM_SYSCALL(&req);

    *err = req.hdr.exit_code;

    // a pointer that isn't a pointer? how dare I?!
    return (char) req.hdr.response;

}

err_t screen_set_color(color_t _color)
{
    screen_color_t req = {
        .hdr.system_call = SYSCALL_SET_SCREEN_COLOR,
        .color = _color
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}

void screen_clear(void)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_CLEAR_SCREEN};
    PERFORM_SYSCALL(&hdr);
}

err_t screen_set_cursor_pos(uint8_t _x, uint8_t _y)
{
    screen_t req = {
        .hdr.system_call = SYSCALL_SET_SCREEN_CURSOR,
        .x = (uint32_t) _x,
        .y = (uint32_t) _y
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.exit_code;
}

void screen_get_cursor_pos(uint16_t _scr_width, uint8_t *_x, uint8_t *_y)
{
    syscall_hdr_t hdr = {.system_call = SYSCALL_GET_SCREEN_CURSOR};
    PERFORM_SYSCALL(&hdr);

    *_x = (uint8_t) (hdr.response % _scr_width);
    *_y = (uint8_t) (hdr.response / _scr_width);
}

void screen_put_char_at(char c, uint8_t _x, uint8_t _y)
{
    char str[2] = {c, 0};

    screen_t req = {
        .hdr.system_call = SYSCALL_SCREEN_PUT_CHAR,
        .x = (uint32_t) _x,
        .y = (uint32_t) _y,
        .str = str
    };
    PERFORM_SYSCALL(&req);
}
