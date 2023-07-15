#include "memory.h"
#include "util.h"
#include "scancode.h"
#include "types.h"
#include "screen.h"
#include "call.h"
#include "ps2keyb.h"
#include "program.h"
#include "disk.h"

#define PROGRAM_NAME    "TEXT"
#include "debug.h"

#include "include/edit.h"

#define PROGRAM_SIZE_ASSUMPTION         100 * 1024 // we assume we are 100 kb
#define ALREADY_ALLOCED_ASSUMPTION      200 * 1024 // assume 200 kb is already alloc'ed
#define KEYB_BFR_ENTRIES                128
#define KEYMAP_PATH                     "/sys/usint.KL"

typedef struct keymap_entry_t
{
    char lc;
    char uc;
    uint16_t scancode;
} __attribute__((packed)) keymap_entry_t;

typedef enum action_t
{
    NO_ACTION,
    EXIT, // CTRL + D
    SAVE, // CTRL + S
} action_t;

char *g_contents = NULL;
keymap_entry_t *g_keymap = NULL;
size_t g_keymap_size = 0;
uint32_t g_iterator = 0;

uint8_t g_edit_flags = 0;

/**
 * @brief registers EDIT as a subscriber to the PS2KEYB driver
 *
 * @param kb_api api space of PS2KEYB driver
 * @param keyb_bfr the keyboard/scancode buffer
 * @param size size of the keyboard/scancode buffer
 */
static void edit_keyb_register(api_space_t kb_api, uint16_t *keyb_bfr, size_t size)
{
    ps2keyb_api_req req = {
        .hdr.system_call = kb_api | PS2KEYB_CALL_REGISTER_SUBSCRIBER,
        .buffer = keyb_bfr,
        .buffer_size = size
    };
    PERFORM_SYSCALL(&req);
}

/**
 * @brief UNregisters EDIT as a subscriber to the PS2KEYB driver
 *
 * @param kb_api api space of PS2KEYB driver
 */
static void edit_keyb_unregister(api_space_t kb_api)
{
    ps2keyb_api_req req = {
        .hdr.system_call = kb_api | PS2KEYB_CALL_DEREGISTER_SUBSCRIBER,
    };
    PERFORM_SYSCALL(&req);
}

/**
 * @brief Converts a keyboard scancode to the character it represents using the keymap
 * 
 * @param scancode keycode of key pressed
 * @param shift_pressed 1 if shift was pressed before this key was pressed, 0 if otherwise
 * @return char character the key represents (e.g., 'a' when KEYCODE_A was pressed)
 */
static char edit_key_to_character(uint16_t scancode, uint8_t shift_pressed)
{
    if(!scancode)
        return 0;
        
    for (uint32_t i = 0; i < g_keymap_size / sizeof(keymap_entry_t); ++i)
        if (scancode == g_keymap[i].scancode)
            return (shift_pressed) ? g_keymap[i].uc : g_keymap[i].lc;

    return 0;
}

/**
 * @brief Converts a key to an action or a character, depending on the scancode
 *          NOTE: When a character key is pressed (e.g., 'a' or 'b') NO_ACTION is returned and the character is returned in 
 *                *line_column. In all other cases, *line_column will not be set.
 * 
 * @param scancode keycode of key pressed
 * @param line_column pointer to the column of the line this character should be placed in (in all cases, column == g_x)
 * @return action_t action to perform based on key input
 */
static action_t edit_key_to_action_or_key(uint16_t scancode, char *line_column)
{
    static uint8_t ctrl_pressed = 0;
    static uint8_t shift_pressed = 0;
    char ch = 0;
    action_t action = NO_ACTION;

    switch (scancode)
    {        
        case KEYCODE_RCTRL:
        case KEYCODE_LCTRL:
            ctrl_pressed = 1;
        break;

        case KEYCODE_RCTRL | KEYCODE_FLAG_KEY_RELEASED:
        case KEYCODE_LCTRL | KEYCODE_FLAG_KEY_RELEASED:
            ctrl_pressed = 0;
        break;

        case KEYCODE_LSHIFT:
        case KEYCODE_RSHIFT:
            shift_pressed = 1;
        break;

        case KEYCODE_LSHIFT | KEYCODE_FLAG_KEY_RELEASED:
        case KEYCODE_RSHIFT | KEYCODE_FLAG_KEY_RELEASED:
            shift_pressed = 0;
        break;

        case KEYCODE_ENTER:
            *(line_column) = '\n';
            edit_print();
        break;

        case KEYCODE_SPACE:
            *(line_column) = ' ';
            edit_print();
        break;

        case KEYCODE_BACKSPACE:
            *(line_column) = '\b';
            edit_print();
        break;

        case KEYCODE_D:
            if (ctrl_pressed)
            {
                action = EXIT;
                break;
            }

            *(line_column) = edit_key_to_character(KEYCODE_D, shift_pressed);
            edit_print();
        break;

        case KEYCODE_S:
            if (ctrl_pressed)
            {
                action = SAVE;
                break;
            }

            *(line_column) = edit_key_to_character(KEYCODE_S, shift_pressed);
            edit_print();

        break;

        default:
            ch = edit_key_to_character(scancode, shift_pressed);

            if(!ch)
                break;

            *(line_column) = ch;
            ctrl_pressed = 0;
            edit_print();
        break;

    }

    return action;
}

/**
 * @brief saves a file to disk, helper for edit_perform_action()
 * 
 * @param path path of the file to save
 * @return err_t exit code returned by fs_write_file()
 */
static err_t edit_save_file(char *path)
{
    // save
    return fs_write_file(path, (file_t *) g_contents, g_iterator, FAT_FILE_ATTRIB_FILE);
}

/**
 * @brief performs action given, helper for edit_perform_input()
 * 
 * @param action action to be performed (e.g., SAVE)
 */
static void edit_perform_action(action_t action, char *path)
{
    err_t err = 0;
    switch(action)
    {
        case SAVE:
            err = edit_save_file(path);
            
            if(err)
                screen_print_at("Failed to save file", 0, 24);
        break;

        case EXIT:
        // EXIT is ignored since it is handled in edit()
        default:
        break;
    }
}

/**
 * @brief Scans the keybuffer and handles any action that comes out of it
 * 
 * @param keyb_bfr pointer to key buffer
 * @param entries number of entries in the key buffer
 * @return uint8_t 0 is returned when the EXIT action needs to be performed
 */
uint8_t edit_perform_input(uint16_t *keyb_bfr, uint32_t entries, char *path)
{
    uint8_t do_run = 1;

    for (uint32_t i = 0; i < entries; ++i)
    {
        if(keyb_bfr[i] == 0)
            continue;

        action_t action = edit_key_to_action_or_key(keyb_bfr[i], &g_contents[g_iterator]);

        if(action == EXIT)
            return 0;

        edit_perform_action(action, path);
        keyb_bfr[i] = 0;
    }

    return do_run;
}

/**
 * @brief Prints char on screen
 * 
 */
void edit_print(void)
{
    char s[2] = {g_contents[g_iterator], 0};
    screen_print(s);
    
    if(g_contents[g_iterator] == '\b' && g_iterator)
        g_iterator--;
    else
        g_iterator++;
}

/**
 * @brief main edit loop
 *
 * @param kb_api api space of PS2KEYB driver
 * @return err_t 0 if success, error code if fail
 */
err_t edit(api_space_t kb_api, char *path)
{
    uint8_t run = 1;
    err_t err = 0;

    program_info_t *prog_info = program_get_info(&err);

    if (err)
        return err;
    
    memory_info_t *mem_info = memory_get_info(&err);

    if(err)
        return err;
    
    size_t mem_alloc = (mem_info->memory_space_kb * 1024) - ((size_t)prog_info->bin_start) - PROGRAM_SIZE_ASSUMPTION - ALREADY_ALLOCED_ASSUMPTION;
    
    g_contents = valloc(mem_alloc);
    assert(g_contents);

    memset(g_contents, mem_alloc, 0);

    uint16_t *keyb_bfr = valloc(KEYB_BFR_ENTRIES * sizeof(uint16_t));
    assert(keyb_bfr);
    edit_keyb_register(kb_api, keyb_bfr, KEYB_BFR_ENTRIES * sizeof(uint16_t));

    if (err)
        return err;

    char *path_keymap = disk_get_bootdisk();
    assert(path_keymap);

    memcpy(&path_keymap[strlen(path_keymap)], KEYMAP_PATH, sizeof(KEYMAP_PATH));

    g_keymap = (keymap_entry_t *) fs_read_file(path_keymap, &g_keymap_size, &err);  
    vfree(path_keymap);  

    if (err)
        return err;
    
    g_iterator = 0;
    while (run)
        run = edit_perform_input(keyb_bfr, KEYB_BFR_ENTRIES, path);

    edit_keyb_unregister(kb_api);
    vfree(g_keymap);
    vfree(keyb_bfr);
    vfree(g_contents);

    return err;
}
