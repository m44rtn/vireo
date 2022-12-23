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


#include "../syslib/lib/include/exit_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define _(a) # a
#define NOTIFY_NOT_IMPLEMENTED printf(_(__LINE__)); printf(": FUNCTION NOT IMPLEMENTED\n");
#define PATH_MAX    255
#define MAX_LINES_IN_FILE   255
#define MAX_CHARS_IN_LINE   255
#define MAX                 0xFFFFFFFF

char g_working_dir[PATH_MAX];

typedef struct keymap_entry_t
{
    char lc;
    char uc;
    uint16_t scancode;
} __attribute__((packed)) keymap_entry_t;

// returns first occurance of the thing to be found
static uint32_t find_in_str(char *o, const char *fnd)
{
	uint32_t c = 0;
	uint32_t len = strlen((char *) fnd);
	uint32_t olen = strlen(o) + 1;

	for(uint32_t i = 0; i < olen; ++i)
	{

		if(o[i] == fnd[c])
			++c;
		else
			c = 0;
		
		if(c == len)
			return (i - len) + 1;
	}

	return MAX;
}

static uint8_t str_get_part(char *part_out, const char *s, const char *delim, uint32_t *pindex)
{
	uint32_t strindex = 0;

	if(*(pindex) == MAX)
		return 0; // done

	for(uint32_t i = 0; i < *(pindex); ++i)
	{
		if((strindex = strindex + find_in_str((char *) (&s[strindex]), delim)) == MAX)
			break;

		strindex = strindex + strlen(delim);
	}

	*(pindex) = *(pindex) + 1;
	
	if(strindex == MAX)
	{
		*(pindex) = MAX;
		return 0;
	}

	uint32_t next = find_in_str((char *) (&s[strindex]), delim);

	if(next == MAX)
	{
		next = strlen(&s[strindex]);
		*(pindex) = MAX;
	}
	
	memcpy(part_out, (void *) (&s[strindex]), next);
	part_out[next] = '\0';

	return 1; // not done
}

static void current_dir(char *path, size_t p_size)
{
    getcwd(path, p_size);
}

static FILE * open_file(const char *file, size_t *o_size)
{
    FILE *f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    *o_size = ftell(f);
    rewind(f);
    
    return f;
}

static uint16_t get_scancode_from_defname(char *name, char *scancodes)
{
    size_t len = strlen(name);
    name[len] = ' ';
    name[len + 1] = '\0';
    uint32_t index = find_in_str(scancodes, name) + len;
    index = index + find_in_str(&scancodes[index], "0x");

    uint32_t res = (uint32_t) strtol(&scancodes[index], NULL, 0);

    // // first get all digits
    // char hex[4] = {0, 0, 0, 0};
    // uint32_t n_digits = 0;
    // for(uint32_t i = 0; i < 4; i++)
    // {
    //     char c = scancodes[index + i];

    //     if(c < '0' || c > '9')
    //         break;
        
    //     hex[n_digits++] = c;   
    // }

    // uint16_t res = 0;
    // n_digits = n_digits - 1;

    // const char *digs = "0123456789abcdef";

    // for(uint32_t i = 0; i < 4; i++)
    // {
    //     if(hex[i] == 0)
    //         break;
        
    //     char *loc = strchr((char *) digs, (int) hex[i]);
    //     uint32_t val = (uint32_t) ((uint32_t)loc - (uint32_t)digs);

    //     res = (uint16_t)(res + ((val) << (n_digits * 4u)));
    //     n_digits--;
    // }

    return res;
}

static uint32_t convert_file(FILE *input, keymap_entry_t *output)
{
    char line[MAX_CHARS_IN_LINE];
    char lc, uc;
    char define_name[MAX_CHARS_IN_LINE / 2];

    uint32_t ln = 0;

    size_t fsize = 0;
    FILE *scancodes = open_file("scancode.h", &fsize);

    char *all_scancodes = malloc(fsize);
    fread(all_scancodes, 1, fsize, scancodes);
    fclose(scancodes);
    
    while(fgets(line, MAX_CHARS_IN_LINE, input) && ln < MAX_LINES_IN_FILE)
    {
        uint32_t pindex = 0;
        str_get_part(define_name, line, " ", &pindex);
        str_get_part(&lc, line, " ", &pindex);
        str_get_part(&uc, line, " ", &pindex);

        output[ln].lc = lc;
        output[ln].uc = uc;
        output[ln].scancode = get_scancode_from_defname(define_name, all_scancodes);
        ln++;
    }

    return ln;
}

int main(uint32_t argc, char **argv)
{
    if(argc < 2)
    {
        printf("Please provide an input file\n");
        return EXIT_CODE_GLOBAL_INVALID;
    }
    
    char *file = argv[1];
    current_dir(g_working_dir, sizeof(g_working_dir));

    size_t fsize = 0;
    FILE *input = open_file(file, &fsize);

    keymap_entry_t *output = malloc(MAX_LINES_IN_FILE * sizeof(keymap_entry_t));
    
    if(!output)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
    
    uint32_t lines = convert_file(input, output);

    char *out_path = malloc(strlen(file) + 3);

    if(!out_path)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
    
    // rename file extension (if input file is KEYBLAYOUT.TXT then output file is called
    // KEYBLAYOUT.KL)
    memcpy(out_path, file, strlen(file));
    uint32_t doti = find_in_str(out_path, ".");

    if(doti == MAX)
        { doti = strlen(out_path); out_path[doti] = '.'; }

    out_path[doti + 1] = 'K';
    out_path[doti + 2] = 'L';
    out_path[doti + 3] = '\0';
    
    FILE * out = fopen(out_path, "w+");
    fwrite(output, sizeof(keymap_entry_t), lines, out);

    fclose(input);
    fclose(out);

    return EXIT_CODE_GLOBAL_SUCCESS;
}
