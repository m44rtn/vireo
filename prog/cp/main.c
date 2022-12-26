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

typedef struct keymap_entry_t
{
    char lc;
    char uc;
    uint16_t scancode;
} __attribute__((packed)) keymap_entry_t;

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
    file_t *cf = config_load_configfile(&err); 

    if(err)
        return err;

    err = config_load_drv(cf);

    if(err)
        return err;

    api_listing_t *list = api_get_syscall_listing();
    api_space_t keyb_api = 0;

    for(uint32_t i = 0; i < 0xFF; ++i)
        if(!strcmp(list[i].filename, "PS2KEYB.DRV"))
        {
            keyb_api = list[i].start_syscall_space;
            break;
        };
    
    if(!keyb_api)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    
    size_t keymap_size = 0;
    keymap_entry_t *keymap = fs_read_file("CD0/SYS/USINT.KL", &keymap_size, &err);

    if(err)
        return err;
    
    syscall_hdr_t req = {.system_call = keyb_api + PS2KEYB_CALL_LAST_KEY};
    char last_char = ' ';
    while(1)
    {
        PERFORM_SYSCALL(&req);
        if(req.response == KEYCODE_UNUSED && (req.response & KEYCODE_FLAG_KEY_RELEASED))
            continue;

        last_char = print_char(req.response, keymap, keymap_size, last_char);
        
    }

    return err;
}
