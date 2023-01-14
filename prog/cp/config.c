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

#include "fs.h"
#include "util.h"
#include "disk.h"
#include "memory.h"
#include "driver.h"

#define PROGRAM_NAME "CP"
#include "debug.h"

#include "include/fileman.h"
#include "include/config.h"

#define CONFIG_PATH     "/CONFIG"

#define MAX_LINE_LEN    1024 // bytes

// standard chars and strings to interpret
#define COMMENT_CHAR    '#'
#define KEYWORD_LOAD_DRV "LOAD"

// CP chars and strings to interpret
#define KEYWORD_KEYB_DRIVER_NAME "KEYB_DRV_NAME"
#define KEYWORD_KEYMAP_PATH "KEYMAP"

file_t *config_read_file(err_t *err)
{
    char *bootdisk = disk_get_bootdisk();

    char path[MAX_PATH_LEN];
    merge_disk_id_and_path(bootdisk, (char *) CONFIG_PATH, path);
    vfree(bootdisk);

    size_t fsize;
    return fs_read_file(path, &fsize, err);  
}

static uint8_t config_get_line(file_t *cf, char *out, uint32_t *pindex)
{
    uint8_t is_not_done = 1;

    while((is_not_done = str_get_part(out, cf, "\n", pindex)))
    {
        to_uc(out, strlen(out));

        if(out[0] == '#')
            continue;
        else if(out[0] < '/' || out[0] > 'z')
            continue;
        else
            break;
    }

    // NOTE: is_not_done will be 1 while we still have lines left, otherwise 0
    return is_not_done;
}

static err_t config_actually_load_driver(char *path, uint32_t n_drv)
{
    // we don't actually need to care about the driver id, as long as its unique and non-zero
    if(path[0] != '/')
        { driver_add(path, n_drv); return EXIT_CODE_GLOBAL_SUCCESS; }

    char *bd = disk_get_bootdisk();
    char *p = valloc(strlen(path) + strlen(bd) + 1);
    merge_disk_id_and_path(bd, path, p);
    
    err_t err = driver_add(p, n_drv);
    vfree(p);

    return err;
}

err_t config_load_drv(file_t *cf)
{
    uint32_t line_num = 0;
    char *line = valloc(MAX_LINE_LEN);
    uint32_t n_drv = 1;

    while(config_get_line(cf, line, &line_num))
    {
        if(strcmp_until(line, KEYWORD_LOAD_DRV, strlen(KEYWORD_LOAD_DRV)))
            continue;

        // remove keyword 'LOAD' from line
        uint32_t spacei = find_in_str(line, " ") + 1;
        memcpy(line, &line[spacei], strlen(&line[spacei]) + 1);
        assert(!config_actually_load_driver(line, n_drv++)); // TODO: report error to user
    }

    return EXIT_CODE_GLOBAL_SUCCESS;
}

char *config_get_keyb_drv_name(file_t *cf)
{
    uint32_t line_num = 0;
    char *line = valloc(MAX_LINE_LEN);
    char *out = valloc(MAX_FILENAME_LEN);

    while(config_get_line(cf, line, &line_num))
    {
        if(strcmp_until(line, KEYWORD_KEYB_DRIVER_NAME, strlen(KEYWORD_LOAD_DRV)))
            continue;

        uint32_t pindex = 1;
        str_get_part(out, line, " ", &pindex);
    }

    vfree(line);
    return out;
}

char *config_get_keymap_path(file_t *cf)
{
    uint32_t line_num = 0;
    char *line = valloc(MAX_LINE_LEN);
    char *out = valloc(MAX_FILENAME_LEN); // + 2 for: (+ 1 '\0', +1 '.' to seperate filename and extension)

    while(config_get_line(cf, line, &line_num))
    {
        if(strcmp_until(line, KEYWORD_KEYMAP_PATH, strlen(KEYWORD_LOAD_DRV)))
            continue;

        uint32_t pindex = 1;
        str_get_part(out, line, " ", &pindex);
    }

    vfree(line);
    return out;
}
