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

#include "include/fileman.h"

#include "disk.h"
#include "memory.h"
#include "util.h"
#include "fs.h"

char working_dir[MAX_PATH_LEN + 1];

/**
 * @brief Returns the full path to a file based on the information in cmd_bfr,
 *        even if the command buffer only contains a file name
 * 
 * @param cmd_bfr Command buffer.
 * @param abspath An absolute path to a directory (e.g., path to the binary
 *                directory)*.
 * @param cwd Current working directory path.
 * @return char* Full path to a file, NULL if file not found.
 * 
 * *= If not needed, it can be filled with the same pointer as cwd or an empty string.
 */
char *fileman_abspath_or_cwd(char *cmd_bfr, const char *abspath, char *cwd)
{
    // If the path is the cmd_bfr, then it contains an absolute path since the
    // FS drivers where able to resolve the path to an actual file. 
    if(fileman_is_existing_file(cmd_bfr))
        return cmd_bfr;

    char *path = valloc(MAX_PATH_LEN);
    char *abs = valloc(MAX_PATH_LEN);
    uint32_t pindex = 0;

    str_get_part(abs, cmd_bfr, " ", &pindex);
    merge_disk_id_and_path(abspath, abs, path);

    // Check if file can be found in the abspath directory
    if(fileman_is_existing_file(path))
        { merge_disk_id_and_path(abspath, cmd_bfr, path); vfree(abs); return path; }
    
    merge_disk_id_and_path(cwd, abs, path);

    // Check if the file can be found in the cwd.
    if(fileman_is_existing_file(path))
        { merge_disk_id_and_path(cwd, cmd_bfr, path); vfree(abs); return path; }
    
    vfree(abs);
    vfree(path);
    return NULL;
}

/**
 * @brief Reads a file from the bootdisk
 * 
 * @param path Path to file on the bootdisk, excluding the drive id
 * @param err Out: error code: any by filesystem.
 * @param fsize Out: file size.
 * @return file_t* Pointer to file buffer, or NULL on error.
 */
file_t *read_file_from_bootdisk(const char *path, err_t *err, size_t *fsize)
{
    char *bootdisk = disk_get_bootdisk();
    char *opath = valloc(strlen(bootdisk) + strlen(path) + 1);

    merge_disk_id_and_path(bootdisk, (char *) path, opath);
    vfree(bootdisk);

    file_t *f = fs_read_file(opath, fsize, err);
    vfree(opath);

    return f;
}

/**
 * @brief Merges a path without a drive id with a drive id, creating an absolute path.
 * 
 * @param disk Drive id string.
 * @param path Path excluding drive id.
 * @param out Out: absolute path.
 * 
 * @example
 * char path[32];
 * merge_disk_id_and_path("CD0", "/HELLO/WORLD", path);
 * 
 * path --> "CD0/HELLO/WORLD"
 */
void merge_disk_id_and_path(const char *disk, const char *path, char *out)
{    
    size_t d_len = strlen(disk);
    memcpy(out, disk, d_len);
    memcpy(&out[d_len], path, strlen(path) + 1);
}

/**
 * @brief Checks the first characters of `path` to check if it
 *        contains a drive id.
 * 
 * @param path Path
 * @return uint8_t 
 *              - 0 if the path does not contain a drive id.
 *              - 1 if the path *does* contain a drive id.
 */
uint8_t fileman_contains_disk(char *path)
{
    // not happy with this, but it works
    if(!(path[2] >= '0' && path[2] <= '9'))
        return 0;
    
    if(!(path[1] >= 'A' && path[1] <= 'Z'))
        return 0;
    
    if(!(path[0] >= 'A' && path[0] <= 'Z'))
        return 0;

    return 1;    
}

/**
 * @brief Checks whether `path` contains a file.
 * 
 * @param path Path
 * @return uint8_t 0 when not a file, 1 when it is a file.
 */
uint8_t fileman_is_existing_file(char *path)
{
    err_t err = 0;
    fs_file_info_t *t = fs_file_get_info(path, &err);

    if(!t || err)
        return 0;
    
    vfree(t);
    return 1;
}

/**
 * @brief Checks whether the file at `path` is a directory
 * 
 * @param path 
 * @return uint8_t 
 *              - 0 if not a directory.
 *              - 1 if a directory.
 */
static uint8_t fileman_is_existing_dir(char *path)
{
    err_t err = 0;
    uint8_t res = 1;
    fs_file_info_t *t = fs_file_get_info(path, &err);

    if(!t)
        res = 0;
    else if(err)
        res = 0;
    else if((t->file_type & FAT_FILE_ATTRIB_DIR) != FAT_FILE_ATTRIB_DIR)
       res = 0;
    
    vfree(t);
    return res;
}

/**
 * @brief Set current working directory
 * 
 * @param path Absolute path to new current working directory
 * @return err_t Error code:
 *                  - EXIT_CODE_GLOBAL_SUCCESS on success.
 *                  - EXIT_CODE_GLOBAL_INVALID when not a valid directory.
 */
err_t setcwd(char *path)
{
    // first check if this path even exists...
    if(!fileman_is_existing_dir(path))
        return EXIT_CODE_GLOBAL_INVALID;

    size_t len = strlen(path) + 1;
    len = (len > MAX_PATH_LEN + 1) ? MAX_PATH_LEN + 1 : len;

    memcpy(working_dir, path, len);

    if(working_dir[len - 2] != '/')
        { working_dir[len - 1] = '/'; working_dir[len] = '\0'; }
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

/**
 * @brief Get current working directory.
 * 
 * @param buf Out: pointer to output buffer.
 * @param len Out: string length of current working directory path.
 */
void getcwd(char *buf, uint32_t *len)
{
    *len = strlen(working_dir);
    memcpy(buf, working_dir, *len + 1);
}
