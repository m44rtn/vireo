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
#include "memory.h"
#include "screen.h"
#include "call.h"
#include "api.h"
#include "ps2keyb.h"
#include "fs.h"

#include "exit_code.h"
#include "util.h"
#include "scancode.h"

#include "include/config.h"
#include "include/keyb.h"

static char print_char(uint16_t keycode, keymap_entry_t *keymap, size_t keymap_size, char l)
{
    keymap_size = keymap_size / sizeof(keymap_entry_t);
    char p = ' ';

    for(uint32_t i = 0; i < keymap_size; ++i)
        if(keymap[i].scancode == keycode)
            p = keymap[i].lc;
    
    if(p == l)
        return p;

    char s[2];
    str_add_val(s, "%c", p);
    screen_print(s);

    return p;
}

err_t main(uint32_t argc, char **argv)
{    
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    file_t *cf = config_read_file(&err); 

    if(err)
        return err;

    err = config_load_drv(cf);

    if(err)
        return err;
    
    err = keyb_start(cf);
    
    if(err)
        return err;
    
    while(1)
    {
        char lc = keyb_get_usable_char();
        char str[2] = {lc, 0};

        if(lc)
            screen_print(str);
    }

    return err;
}
