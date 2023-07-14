/*
MIT license
Copyright (c) 2023 Maarten Vermeulen

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
#include "exit_code.h"
#include "screen.h"
#include "fs.h"
#include "cp.h"
#include "api.h"
#include "util.h"
#include "memory.h"

#define CP_BIN_NAME     "CP.ELF"
#define KEYB_BIN_NAME   "PS2KEYB.DRV"

#define FLAG_READ_ONLY_MODE     (1 << 0)

uint8_t g_flags = 0;

/**
 * @brief Get the api space of the command program (shell)
 * 
 * @return api_space_t start of api space of CP, or 0 if not found.
 */
static api_space_t get_api_space(char *fname)
{
    api_listing_t *list = api_get_syscall_listing();
    api_space_t s = 0x0000;

    for(uint32_t i = 0; i < (API_LAST_CALL / API_SPACE_SEGMENT_SIZE); ++i)
    {
        if(!list[i].filename[0])
            continue;

        if(strcmp(fname, list[i].filename))
            continue;

        s = list[i].start_syscall_space;
    }

    vfree(list); 
    return s;
}

/**
 * @brief Gets the current working directory, as reported by CP. Helper for main().
 * 
 * @param cp_api start of api space of CP
 */
static char * get_wd(api_space_t cp_api)
{
    cp_api_req req = {
        .hdr.system_call = (syscall_t) (cp_api + CP_GET_CWD),
    };
    PERFORM_SYSCALL(&req);

    return req.hdr.response_ptr;
}

/**
 * @brief main function
 * 
 * @param argc 
 * @param argv 
 * @return err_t:
 *          - EXIT_CODE_GLOBAL_SUCCESS, on success
 *          - EXIT_CODE_GLOBAL_INVALID, if the user input was invalid (no filename was given)
 *          - EXIT_CODE_GLOBAL_NOT_INITIALIZED, when the api space of CP was not found
 *          - filesystem errors when writing/reading files failed
 */
err_t main(uint32_t argc, char **argv)
{    
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;

    if(argc < 2)
        { screen_print("please provide a filename within the current directory.\nUsage: text.elf [path]\n"); return EXIT_CODE_GLOBAL_INVALID; }
    
    api_space_t cp_api = get_api_space(CP_BIN_NAME);

    if(!cp_api)
        return EXIT_CODE_GLOBAL_NOT_INITIALIZED;
    
    char *path = get_wd(cp_api);

    // path is stored in a page and the maximum file path allowed by CP is 256 chars, so we have enough space
    // to do this even without checking if we do.
    memcpy(&path[strlen(path)], argv[1], strlen(argv[1]) + 1);

    uint8_t fs_type = fs_get_filesystem(path);

    if(fs_type == FS_TYPE_ISO)
        return EXIT_CODE_FS_FILE_READ_ONLY;

    api_space_t kb_api = get_api_space(KEYB_BIN_NAME);
    err = edit(kb_api, path);

    vfree(path);
    screen_clear();
    return err;
}
