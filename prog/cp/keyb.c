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

#include "exit_code.h"
#include "call.h"
#include "api.h"
#include "ps2keyb.h"
#include "util.h"
#include "scancode.h"
#include "disk.h"

#include "include/keyb.h"
#include "include/fileman.h"
#include "include/config.h"
#include "include/cp_exit_codes.h"

#define KEYBOARD_BFR_SIZE   1024 // bytes (512 keycodes)

api_space_t g_keyb_api = (uint16_t) MAX;
char g_keyb_drv_name[MAX_FILENAME_LEN];
keymap_entry_t *g_keymap = NULL;
size_t g_keymap_size = 0;

uint16_t *g_keyb_bfr = NULL;

static api_space_t keyb_get_api_space(char *keyb_driver_name)
{
    api_listing_t *list = api_get_syscall_listing();
    api_space_t keyb_api = 0;

    for(uint32_t i = 0; i < 0xFF; ++i)
        if(!strcmp(list[i].filename, g_keyb_drv_name))
        {
            keyb_api = list[i].start_syscall_space;
            break;
        };
    
    if(!keyb_api)
        return (uint16_t) MAX;
    
    return keyb_api;
}

static keymap_entry_t * keyb_load_keymap(file_t *cf)
{
    char *path = config_get_keymap_path(cf);

    size_t keymap_size = 0;
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;

    char *p = valloc(MAX_PATH_LEN);
    char *bd = disk_get_bootdisk();
    merge_disk_id_and_path(bd, path, p);
    vfree(path);

    keymap_entry_t *keymap = fs_read_file(p, &g_keymap_size, &err);

    vfree(p);
    
    if(err)
        return NULL;
    
    return keymap;
}

err_t keyb_start(file_t *cf)
{
    char *drv_name = config_get_keyb_drv_name(cf);
    memcpy(g_keyb_drv_name, drv_name, MAX_FILENAME_LEN);
    vfree(drv_name);

    g_keyb_api = keyb_get_api_space(g_keyb_drv_name);

    if(g_keyb_api == (uint16_t) MAX)
        return EXIT_CODE_CP_NO_KEYB_DRV;
    
    g_keymap = keyb_load_keymap(cf);

    if(!g_keymap)
        return EXIT_CODE_CP_NO_KEYMAP;
    
    // FIXME/TODO: some parts of this program might benefit from using a shared memory pool.
    g_keyb_bfr = valloc(KEYBOARD_BFR_SIZE);

    if(!g_keyb_bfr)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;

    ps2keyb_api_req req = {
        .hdr.system_call = g_keyb_api + PS2KEYB_CALL_REGISTER_SUBSCRIBER,
        .buffer = g_keyb_bfr,
        .buffer_size = KEYBOARD_BFR_SIZE
        };
    PERFORM_SYSCALL(&req);

    if(req.hdr.exit_code)
        return req.hdr.exit_code;
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

static char keyb_in_keymap(uint16_t code)
{
    for(uint32_t i = 0; i < g_keymap_size / sizeof(keymap_entry_t); ++i)
        if(code == g_keymap[i].scancode)
            return g_keymap[i].lc;
    
    return 0;
}

static char keyb_convert_keycode(uint16_t code)
{
    char lc = 0;
    
    if(code == KEYCODE_ENTER)
        { memset(g_keyb_bfr, KEYBOARD_BFR_SIZE, 0); lc = '\n'; }
    else if(code == KEYCODE_SPACE)
        lc = ' ';
    else if(code == KEYCODE_BACKSPACE)
        lc = '\b';
    else
        lc = keyb_in_keymap(code);
    
    return lc;
}

uint32_t keyb_get_character(char *bfr)
{
    if(!g_keyb_bfr[0])
        return 0;

    char lc = 0;
    uint32_t n = 0;

    for(uint32_t i = 0; i < KEYBOARD_BFR_SIZE / sizeof(uint16_t); ++i)
    {
        lc = keyb_convert_keycode(g_keyb_bfr[i]);

        // if(lc == '\b' && n > 0)
        //     n = n - 1;
        if(lc)
            bfr[n++] = lc;
    }

    memset(g_keyb_bfr, KEYBOARD_BFR_SIZE, 0);

    return n;
}
