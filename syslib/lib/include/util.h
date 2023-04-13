/*
MIT license
Copyright (c) 2021-2022 Maarten Vermeulen

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

#include "types.h"
#include "exit_code.h"
#include "memory.h"

#define HEX_DIGITS_USE_DEFAULT      0

// returns length of string
unsigned int strlen(const char *str);

// removes chars in a string starting at p for n 
void remove_from_str(char *p, unsigned int n);

// replaces chars matching c with repl
void replace_in_str(char *p, const char c, const char repl);

// compares two strings and returns 0 if they are equal
unsigned char strcmp(const char *str1, const char *str2);

// compares two strings until `stop` bytes have been checked, returns 0 if equal
unsigned char strcmp_until(const char *str1, const char *str2, unsigned int stop);

// allocates a backup string from str using valloc
char *create_backup_str(const char *str);

// makes lowercase characters in string s, of size slen, uppercase (will manipulate original string)
void to_uc(char *s, size_t slen);

// makes uppercase characters in string s, of size slen, lowercase (will manipulate original string)
void to_lc(char *s, size_t slen);

// will flip cases of lower- and uppercase characters in string s, of size slen
void to_other_case(char *s, size_t slen);

// creates a string containing value in hexadecimal format (without leading `0x`),
// amount of digits can be controlled with digit_amount. setting digit_amount to 0 is the same
// as setting it to 8. digit_amount values are not strictly respected, meaning that when more
// digits are necessarry to represent the value in parameter value the value in digit_amount 
// WILL be ignored 
// (i.e. hex_digit_count(value) > digit_amount then digit_amount = hex_digit_count(value)).
char* hexstr(unsigned int value, unsigned char digit_amount);

// creates a string containing value in decimal format
char *intstr(unsigned int value);

// converts an ascii number to a integer value (i.e. '9' becomes 9)
unsigned char strdigit_to_int(const char digit);

// returns amount of decimal digits necessary to represent value
unsigned int digit_count(unsigned int value);

// returns amount of hexadecimal digits necessary to represent value
unsigned int hex_digit_count(unsigned int value);

// places val at start for size bytes
void memset(void *start, size_t size, char val);

// returns 1 if to_check is set in flag
unsigned char flag_check(unsigned int flag, unsigned int to_check); 

// finds fnd in string o and returns index at which fnd can be found
// returns MAX if not found
unsigned int find_in_str(char *o, const char *fnd);

// searches for first occurance of ch
unsigned char strchr(char *str, char ch);

// copies size bytes of memory from source to destination
void memcpy(void *destination, const void *source, unsigned int size);

// adds a value to string str complying to format (comparable to sprintf() except this one is only 
// capable of adding one value at a time).
// format:
//  %s adds string
//  %i adds integer value
//  %x adds hexadecimal value
//  %c adds a char
void str_add_val(char *str, const char *format, unsigned int value);

// returns the first bit that is set, function will search for size bits (max 32)
unsigned char nth_bit(uint32_t dword, uint8_t size);

// returns a part of the string from start to the first chars equal to delim found in part_out.
// function returns 0 when it has reached the end of the string (s) or 1 when it was not
// *pindex is for internal use of the function but can be used to specify which part is wanted.
//
// example: input: s = "THIS/IS/A/PATH/TO/A/FILE"
//          input: delim = "/"
//          input: *(&pindex) = 1 
//    then:
//          output: *part_out = "IS" 
//          output: returns uint8_t 1 for not at end of string    
//
// description of input:
//
//      part_out, pointer to copy the string part to
//      s, original string
//      delim, string specifying the characters that seperate a part (delimiter)
//      pindex, pointer to uint32_t (used internally) / specifies which part is wanted
uint8_t str_get_part(char *part_out, const char *s, const char *delim, uint32_t *pindex);

#endif