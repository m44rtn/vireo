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
#include "disk.h"
#include "kernel.h"

#include "include/keyb.h"
#include "include/fileman.h"
#include "include/config.h"
#include "include/cp_exit_codes.h"

#define KEYBOARD_BFR_SIZE   1024 // bytes (512 keycodes)

#define KEYB_FLAG_SHIFT     (1 << 0) // shift pressed

api_space_t g_keyb_api = (uint16_t) MAX;
char g_keyb_drv_name[MAX_FILENAME_LEN];
keymap_entry_t *g_keymap = NULL;
size_t g_keymap_size = 0;

uint16_t *g_keyb_bfr = NULL;
uint16_t g_last_keycode = 0;    /* Last pressed keycode */

uint8_t g_flags = 0;

/**
 * @brief Find keyboard driver API.
 * 
 * @return api_space_t API space (API code range) of the keyboard driver.
 */
static api_space_t keyb_get_api_space(void)
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

/**
 * @brief Loads the keymap at the location provided by the system
 *        configuration file.
 * 
 * @param cf Pointer to system configuration file buffer.
 * @return keymap_entry_t* Pointer to the cached keymap.
 *                         NOTE: Size of keymap is stored in g_keymap_size.
 */
static keymap_entry_t * keyb_load_keymap(file_t *cf)
{
    char *path = config_get_keymap_path(cf);

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

/**
 * @brief Initialize use of keyboard:
 *          - Finds the keyboard driver API.
 *          - Subscribes to the keyboard driver.
 *          - Reads and caches the keymap.
 * 
 * @param cf Pointer to system configuration file buffer.
 * @return err_t Error code:
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success.
 *                  - EXIT_CODE_CP_NO_KEYB_DRV when the keyboard driver 
 *                    was not found.
 *                  - EXIT_CODE_CP_NO_KEYMAP when the keymap was not found.
 *                  - Any error returned by the keyboard driver on 
 *                    subscribe attempt.
 */
err_t keyb_start(file_t *cf)
{
    char *drv_name = config_get_keyb_drv_name(cf);
    memcpy(g_keyb_drv_name, drv_name, MAX_FILENAME_LEN);
    vfree(drv_name);

    g_keyb_api = keyb_get_api_space();

    if(g_keyb_api == (uint16_t) MAX)
        return EXIT_CODE_CP_NO_KEYB_DRV;
    
    g_keymap = keyb_load_keymap(cf);

    if(!g_keymap)
        return EXIT_CODE_CP_NO_KEYMAP;

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

/**
 * @brief Looks through the keymap and returns the correct character for
 *        a specific keycode. Helper for keyb_convert_keycode().
 * 
 * @param code Keycode
 * @return char Key converted to character (upper-/lowercase depending on SHIFT)
 *              or 0 if not found in the keymap.
 */
static char keyb_in_keymap(uint16_t code)
{
    for(uint32_t i = 0; i < g_keymap_size / sizeof(keymap_entry_t); ++i)
        if(code == g_keymap[i].scancode)
            return (g_flags & KEYB_FLAG_SHIFT) ? g_keymap[i].uc : g_keymap[i].lc;
    
    return 0;
}

/**
 * @brief Converts any keycode to a character or internal function flag.
 *        Helper for keyb_get_character().
 * 
 * @param code Keycode
 * @return char Character pressed by user, or 0 if it was converted to an
 *              internal function flag or an unsupported key.
 */
static char keyb_convert_keycode(uint16_t code)
{
    if(!code)
        return 0;

    char lc = 0;
    g_last_keycode = code;

    switch(code)
    {
        case KEYCODE_ENTER:
            memset(g_keyb_bfr, KEYBOARD_BFR_SIZE, 0); 
            lc = '\n';
        break;

        case KEYCODE_SPACE:
            lc = ' ';
        break;

        case KEYCODE_BACKSPACE:
            lc = '\b';
        break;

        case KEYCODE_RSHIFT:
        case KEYCODE_LSHIFT:
            g_flags |= KEYB_FLAG_SHIFT;
        break;

        case (KEYCODE_FLAG_KEY_RELEASED | KEYCODE_LSHIFT):
        case (KEYCODE_FLAG_KEY_RELEASED | KEYCODE_RSHIFT):
            g_flags &= (uint8_t) ~(KEYB_FLAG_SHIFT);
        break;

        default:
            lc = keyb_in_keymap(code);
        break;
    }
    
    return lc;
}

/**
 * @brief Get all characters pressed since the last keyboard buffer clear.
 * 
 * @param bfr Pointer to buffer that will store the pressed characters.
 * @return uint32_t Number of characters pressed by user.
 */
uint32_t keyb_get_character(char *bfr)
{
    // Assume the keyboard buffer is empty if the first scancode in
    // the buffer is 0 (KEYCODE_UNUSED);
    if(!g_keyb_bfr[0])
        return 0;

    char lc = 0;
    uint32_t n = 0;

    for(uint32_t i = 0; i < KEYBOARD_BFR_SIZE / sizeof(uint16_t); ++i)
    {
        lc = keyb_convert_keycode(g_keyb_bfr[i]);

        if(lc)
            bfr[n++] = lc;
    }

    memset(g_keyb_bfr, KEYBOARD_BFR_SIZE, 0);

    return n;
}

/**
 * @brief Returns the last pressed key as a character.
 *        NOTE: Only works for keys that are actually converted to characters
 *              by keyb_convert_keycode() and keyb_in_keymap(). Since keyb_in_keymap()
 *              checks for SHIFT presses when converting a keycode to a character, it is 
 *              possible that the buffer contains both upper- and lowercase characters.
 * 
 * @return char Last character pressed by user.
 */
char keyb_get_last_pressed(void)
{
    char *bfr = valloc(KEYBOARD_BFR_SIZE);

    if(!bfr)
        return 0;

    uint32_t n = keyb_get_character(bfr);
    to_uc(bfr, n);

    char c = bfr[n - 1];
    vfree(bfr);

    return c;
}

/**
 * @brief Waits until the user has pressed a specific key.
 * 
 * @param code Keycode to wait for.
 */
void keyb_wait_for_keycode(uint16_t code)
{
    // clear the last keycode, since we will wait for a new one
    g_last_keycode = 0;
    
    while(g_last_keycode != code)
        (void)keyb_get_last_pressed();
}

/**
 * @brief Empties the keyboard buffer.
 * 
 */
void keyb_empty_buffer(void)
{
    memset(g_keyb_bfr, KEYBOARD_BFR_SIZE, 0);
}
