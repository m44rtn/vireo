/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

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

#ifndef __UTIL_H__
#define __UTIL_H__

unsigned int strlen(const char *str);
void remove_from_str(char *p, unsigned int n);
void replace_in_str(char *p, const char c, const char repl);
unsigned char strcmp(const char *str1, const char *str2);
unsigned char strcmp_until(const char *str1, const char *str2, unsigned int stop);
char *create_backup_str(char *str);

char* hexstr(unsigned int value, unsigned char digit_amount);
char *intstr(unsigned int value);
unsigned char strdigit_toInt(const char digit);

unsigned int digit_count(unsigned int value);
unsigned int hex_digit_count(unsigned int value);

void memset(void *start, unsigned int size, char val);

void sleep(unsigned int timeIn_ms);
unsigned char flag_check(unsigned int flag, unsigned int to_check); 
unsigned int find_in_str(char *o, const char *fnd);

unsigned char strchr(char *str, char ch);
void memcpy(char *destination, char *source, unsigned int size);

void str_add_val(char *str, const char *format, unsigned int value);

unsigned char nth_bit(unsigned int dword, unsigned char size);

char *strtok(char *s, const char *delim);
char *strsep(char **stringp, const char *delim);
char *strpbrk(const char *s, const char *accept);

#endif