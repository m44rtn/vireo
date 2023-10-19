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

/**
 * @brief Copies a directory
 * 
 * @param path path of directory to copy
 * @param copy_to path to copy directory to (+ new directory name)
 * @param attrib file attributes
 * @return err_t 
 */
static err_t copy_dir(char *path, char *copy_to)
{
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    size_t size = 0;

    // make a new directory
    err = fs_mkdir(copy_to);

    if(err)
        return err;

    uint32_t n_entries = 0;
    fs_dir_contents_t *contents = fs_dir_get_contents(path, &n_entries, &err);

    if(err)
        return err;
    
    char *dir_file_path = valloc(FS_MAX_PATH_LEN);
    char *to_dir_file_path = valloc(FS_MAX_PATH_LEN);

    if(!dir_file_path || !to_dir_file_path)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
    
    size_t dir_path_end = strlen(path);
    memcpy(dir_file_path, path, dir_path_end + 1);
    dir_file_path[dir_path_end++] = '/';

    size_t to_dir_path_end = strlen(copy_to);
    memcpy(to_dir_file_path, copy_to, dir_path_end + 1);
    to_dir_file_path[to_dir_path_end++] = '/';

    char *copy_message = valloc(256);
    
    // starts at two to skip '.' and '..' entries
    for(uint32_t i = 2; i < n_entries; ++i)
    {
        memcpy(&dir_file_path[dir_path_end], contents[i].name, strlen(contents[i].name) + 1);
        memcpy(&to_dir_file_path[to_dir_path_end], contents[i].name, strlen(contents[i].name) + 1);

        if(copy_message)
        {
            str_add_val(copy_message, "Copying: %s\n", (uint32_t) dir_file_path);
            screen_print(copy_message);
        }
        
        file_t *file = fs_read_file(dir_file_path, &size, &err); 
        
        if(file && !err)
            err = fs_write_file(to_dir_file_path, file, size, contents[i].attrib);
        
        if(err)
        {
            screen_print("Failed to copy file ");
            screen_print(dir_file_path);
            screen_print("\n");
        } 

        vfree(file);
    }
    
    vfree(copy_message);
    vfree(dir_file_path);
    vfree(to_dir_file_path);

    return err;
}

/**
 * @brief Copies a file
 * 
 * @param path path of file to copy
 * @param copy_to path to copy file to (+ filename)
 * @param attrib file attributes
 * @return err_t 
 */
static err_t copy_file(char *path, char *copy_to, uint8_t attrib)
{
    err_t err = EXIT_CODE_GLOBAL_SUCCESS;
    size_t size = 0;
    file_t *file = fs_read_file(path, &size, &err);

    if(err)
        { screen_print("failed to read file.\n"); return err; }
    
    fs_write_file(copy_to, file, size, attrib);
    vfree(file);

    return err;
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

    fs_file_info_t *info = fs_file_get_info(cwd, &err);

    if(err)
        { screen_print("failed to get original file attributes.\n"); return err; }

    uint8_t attrib = info->file_type;
    vfree(info);

    if(attrib & FAT_FILE_ATTRIB_DIR)
        err = copy_dir(cwd, argv[2]);
    else
        err = copy_file(cwd, argv[2], attrib);

    vfree(cwd);
    return err;
}
