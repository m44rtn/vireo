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

#define CP_BIN_NAME     "CP.ELF"

static api_space_t get_cp_api_space(void)
{
    api_listing_t *list = api_get_syscall_listing();
    api_space_t s = 0x0000;

    for(uint32_t i = 0; i < (API_LAST_CALL / API_SPACE_SEGMENT_SIZE); ++i)
    {
        if(!list[i].filename[0])
            continue;

        if(strcmp(CP_BIN_NAME, list[i].filename))
            continue;

        s = list[i].start_syscall_space;
    }

    vfree(list); 
    return s;
}

err_t main(uint32_t argc, char **argv)
{    
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;

    if(argc < 3)
        { screen_print("Usage: copy.elf [name of file or directory to copy] [full path to copy to]\n"); return EXIT_CODE_GLOBAL_INVALID; }

    api_space_t cp_space = get_cp_api_space();

    cp_api_req req = {
        .hdr.system_call = (syscall_t) (cp_space + CP_GET_CWD),
    };
    PERFORM_SYSCALL(&req);
    
    size_t cwd_len = req.hdr.response_size;
    char *cwd = valloc(cwd_len + FS_MAX_PATH_LEN);
    memcpy(cwd, req.hdr.response_ptr, cwd_len + 1);
    
    vfree(req.hdr.response_ptr);

    memcpy(&cwd[cwd_len], argv[1], strlen(argv[1]) + 1);

    size_t size = 0;
    file_t *file = fs_read_file(cwd, &size, &err);

    if(err)
        { screen_print("failed to read file.\n"); return err; }
    
    fs_file_info_t *info = fs_file_get_info(cwd, &err);

    if(err)
        { screen_print("failed to get original file attributes.\n"); return err; }

    uint8_t attrib = info->file_type;
    vfree(info);
    
    fs_write_file(argv[2], file, size, attrib);
    vfree(file);

    vfree(cwd);
    return err;
}
