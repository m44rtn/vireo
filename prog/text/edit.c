#include "memory.h"
#include "util.h"
#include "scancode.h"
#include "types.h"
#include "screen.h"
#include "call.h"
#include "ps2keyb.h"

#define PROGRAM_NAME    "TEXT"
#include "debug.h"

#include "include/edit.h"

#define MAX_LINE_LEN            80
#define MAX_LINES_P_PAGE        PAGE_SIZE / MAX_LINE_LEN
#define MAX_CONTENTS            512 // ~41k chars (MAX_LINE_LEN * MAX_CONTENTS)
#define KEYB_BFR_ENTRIES        128 // scancodes

#define FLAG_EDIT_SINCE_SAVE    (1 << 0)

typedef struct keymap_entry_t
{
    char lc;
    char uc;
    uint16_t scancode;
} __attribute__((packed)) keymap_entry_t;

typedef struct file_content_t
{
    char line[MAX_LINE_LEN];
} __attribute__((packed)) file_content_t;

typedef enum action_t
{
    NO_ACTION,
    EXIT, // CTRL + D
    SAVE, // CTRL + S
} action_t;

file_content_t *contents[MAX_CONTENTS];
keymap_entry_t *g_keymap = NULL;
size_t g_keymap_size = 0;
uint32_t g_x = 0, g_y = 0;

uint8_t g_edit_flags = 0;

/**
 * @brief Converts a file_t to file_content_t
 *
 * @param file file to edit
 * @return err_t 0 on success, error code if fail
 */
err_t edit_convert_file_to_content(file_t *file)
{
    uint32_t line = 0;
    void *current_line = valloc(PAGE_SIZE);
    uint32_t content_line = 0;

    while (str_get_part(current_line, file, "\n", &line))
    {
        if (content_line >= MAX_CONTENTS)
            return EXIT_CODE_GLOBAL_OUT_OF_RANGE;

        size_t len = strlen(current_line);
        len = (len > MAX_LINE_LEN) ? MAX_LINE_LEN : len;

        memcpy(contents[content_line]->line, current_line, len);
        content_line++;
    }

    return EXIT_CODE_GLOBAL_SUCCESS;
}

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
            shift_pressed = !shift_pressed;
        break;

        case KEYCODE_UPCURSOR:
            g_y = (g_y) ? g_y - 1 : 0;
        break;

        case KEYCODE_DOWNCURSOR:
            g_y = (g_y < MAX) ? g_y + 1 : MAX;
        break;

        case KEYCODE_LEFTCURSOR:
            g_x = (g_x) ? g_x - 1 : 0;
        break;

        case KEYCODE_RIGHTCURSOR:
            g_x = (g_x < MAX_LINE_LEN) ? g_x + 1 : MAX_LINE_LEN;
        break;

        case KEYCODE_D:
            if (ctrl_pressed)
            {
                action = EXIT;
                break;
            }

            *(line_column) = edit_key_to_character(KEYCODE_D, shift_pressed);
            g_edit_flags |= FLAG_EDIT_SINCE_SAVE;
            g_x = (g_x < MAX_LINE_LEN) ? g_x + 1 : MAX_LINE_LEN;
        break;

        case KEYCODE_S:
            if (ctrl_pressed)
            {
                action = SAVE;
                break;
            }

            *(line_column) = edit_key_to_character(KEYCODE_S, shift_pressed);
            g_edit_flags |= FLAG_EDIT_SINCE_SAVE;
            g_x = (g_x < MAX_LINE_LEN) ? g_x + 1 : MAX_LINE_LEN;
        break;

        default:
            ch = edit_key_to_character(scancode, shift_pressed);

            if(!ch)
                break;
            
            *(line_column) = ch;
            g_edit_flags |= FLAG_EDIT_SINCE_SAVE;
            g_x = (g_x < MAX_LINE_LEN) ? g_x + 1 : MAX_LINE_LEN;
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
    char *f = valloc(MAX_CONTENTS * MAX_LINE_LEN);
    assert(f);

    // prepare the file
    for(uint32_t i = 0; i < MAX_CONTENTS; ++i)
        memcpy(&f[strlen(f)], contents[i]->line, strlen(contents[i]->line));
    
    // save
    return fs_write_file(path, (file_t *) f, strlen(f) + 1, FAT_FILE_ATTRIB_FILE);
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

            g_edit_flags = (err) ? g_edit_flags : g_edit_flags & ~(FLAG_EDIT_SINCE_SAVE);
        break;

        case EXIT:
        // EXIT is ignored since it is handled elsewhere
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
        action_t action = edit_key_to_action_or_key(keyb_bfr[i], &contents[g_y]->line[g_x]);

        if(action == EXIT)
            do_run = 0;

        edit_perform_action(action, path);

        if (keyb_bfr[i] == KEYCODE_ENTER)
            { g_x = 0; g_y++; }

        keyb_bfr[i] = 0;
    }

    return do_run;
}

/**
 * @brief Prints char at current g_y and g_x on screen
 * 
 */
void edit_print(void)
{
    if(!contents[g_y]->line[g_x])
        return;
    
    screen_put_char_at(contents[g_y]->line[g_x], g_x, g_y % 20);
    screen_set_cursor_pos(g_x, g_y % 20);
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
    screen_clear();

    err_t err = 0;

    screen_info_t *scr_info = screen_get_info(&err);

    if (err)
        return err;
    
    uint32_t scr_width = scr_info->width;
    // screen_get_cursor_pos(scr_width, (uint8_t *)&g_x, (uint8_t *)&g_y);
    vfree(scr_info);

    uint16_t *keyb_bfr = valloc(KEYB_BFR_ENTRIES * sizeof(uint16_t));
    assert(keyb_bfr);
    edit_keyb_register(kb_api, keyb_bfr, KEYB_BFR_ENTRIES * sizeof(uint16_t));

    if (err)
        return err;

// FIXME: use bootdisk
#define KEYMAP_PATH "CD0/sys/usint.KL"
    g_keymap = (keymap_entry_t *) fs_read_file(KEYMAP_PATH, &g_keymap_size, &err);    

    if (err)
        return err;

    screen_set_color(SCREEN_COLOR_BLACK | (SCREEN_COLOR_LIGHT_GRAY << 4));
    screen_print_at("\t\t CTRL+D: EXIT \t\t CTRL+S: SAVE ", 0, 22);
    screen_set_color(SCREEN_COLOR_LIGHT_GRAY);
    screen_print_at(" \b", 0, 0);

    uint8_t wait_for_save_question_mark = 0;
    g_x = g_y = 0;
    while (run)
    {
        run = edit_perform_input(keyb_bfr, 128, path);

        if(!run && (g_edit_flags & FLAG_EDIT_SINCE_SAVE) && !wait_for_save_question_mark)
        {
            screen_print_at("File was editted since last save, press CTRL+D again to confirm", 0, 24); 
            wait_for_save_question_mark++;
        }

        edit_print();
    }

    edit_keyb_unregister(kb_api);
    vfree(g_keymap);
    vfree(keyb_bfr);

    return err;
}
