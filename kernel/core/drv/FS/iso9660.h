/*
MIT license
Copyright (c) 2019-2022 Maarten Vermeulen

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

#ifndef __ISO9660_H__
#define __ISO9660_H__

#include "../FS_TYPES.H"

void iso_handler(unsigned int *drv);

void iso_init(unsigned char drive);
void iso_search_descriptor(unsigned char drive, unsigned char * buffer, unsigned char type);
void iso_save_pvd_data(uint8_t * pvd, void *info_ptr);

void * iso_allocate_bfr(unsigned int size);
void iso_free_bfr(void *ptr);

unsigned int iso_alloc_dir_buffer(unsigned int dir_size, unsigned int **ret_addr);
unsigned int iso_get_dir_size(unsigned char drive, unsigned int dir_lba);

unsigned int *iso_search_in_path_table(unsigned char drive, char *filename, unsigned char reset);
unsigned int *iso_find_index(unsigned char drive, unsigned short index);

unsigned short iso_search_path_table(const char *file, unsigned char drive, unsigned int start_lba, unsigned int *b_ptr);
unsigned int iso_path_to_dir_lba(unsigned char drive, const char *path);

void reverse_path(char *path, char *out_filename);

unsigned short *iso_read_drive(unsigned char drive, unsigned int lba, unsigned int sctr_read);
void iso_read(char * path, unsigned int *drv);
fs_file_info_t *iso_get_file_info(const char *path);
fs_dir_contents_t *iso_get_dir_contents(const char *path, uint32_t *drv);

#endif