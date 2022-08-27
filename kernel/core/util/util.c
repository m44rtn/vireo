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

#include "util.h"

#include "../dbg/dbg.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../hardware/timer.h"

#include "../screen/screen_basic.h"

#include "../memory/memory.h"

#define UTIL_POOL_SIZE		32

// --> memory pool for util
// this is necessarry because util is used while
// the memory module hasn't been initialized yet
char utilPool[UTIL_POOL_SIZE];

static size_t __strxspn(const char *s, const char *map, char parity);

unsigned int strlen(const char *str)
{
	if(!str)
		return 0;

    uint32_t i = 0;
    while(str[i])
        ++i;

    return i;
}

void remove_from_str(char *p, uint32_t n)
{
	uint32_t max = strlen(p);
	uint32_t i = 0, s = n;

	for(; s < max; ++s)
	{
		p[i] = p[s];
		i++;
	}

	p[max-n] = '\0';
}

void replace_in_str(char *p, const char c, const char repl)
{
	uint32_t max = strlen(p);

	for(uint32_t i = 0; i < max; ++i)
		if(p[i] == c)
			p[i] = repl;
}

unsigned char strcmp(const char *str1, const char *str2)
{
	uint32_t i = 0;

	if(strlen(str1) != strlen(str2))
		return EXIT_CODE_GLOBAL_GENERAL_FAIL;

	while(str1[i] && str2[i])
	{
		if(str1[i] != str2[i])
			return EXIT_CODE_GLOBAL_GENERAL_FAIL;
		++i;
	}

	return EXIT_CODE_GLOBAL_SUCCESS;
}

uint8_t strcmp_until(const char *str1, const char *str2, uint32_t stop)
{
	uint32_t i = 0;

	while((str1[i] && str2[i]) && (i < stop))
	{
		if(str1[i] != str2[i])
			return EXIT_CODE_GLOBAL_GENERAL_FAIL;
		++i;
	}

	// fix for failing if first character in string is '\0' 
	return (str1[0] != str2[0]) ? EXIT_CODE_GLOBAL_GENERAL_FAIL : EXIT_CODE_GLOBAL_SUCCESS;
}

// creates a backup string that can be used for strtok
char *create_backup_str(const char *str)
{
	char *backup;
	backup = kmalloc(strlen(str) + 1);

	if(!backup)
		return NULL; // error (out of mem)

	memcpy(backup, (void *) (str), strlen(str) + 1);

	return backup;
}

/* digit_amount: amount of digits to show, if 0 (or the value is bigger) the normal 
 amount of digits is used. meaning enough to show the actual value.*/
char* hexstr(unsigned int value, uint8_t digit_amount)
{
	unsigned int tempval = value;
	char *outputstr = (char *) &utilPool[0];
	uint8_t digits = (!digit_amount || digit_amount < hex_digit_count(value))? (uint8_t) hex_digit_count(value) : digit_amount;

	uint8_t chrIndex;
	const char* hexDig = "0123456789ABCDEF";

	for(uint8_t i = digits; i > 0; i--){
		chrIndex = tempval & 0x0000000F;
		
		outputstr[i - 1] = hexDig[chrIndex];
		tempval = tempval >> 4;
	}

	outputstr[digits] = '\0';
	
	return outputstr;
}

char *intstr(uint32_t value) 
{          
	char *ReturnString = (char *) &utilPool[0];
	int i = (int) digit_count(value);
	uint32_t NullChar = digit_count(value);
	

	for(i = i - 1; i >= 0; i--)
	{
		ReturnString[i] = (char) (value % 10 + '0');
		value = value / 10;
	}	

	ReturnString[NullChar] = '\0';

	return ReturnString;
}


uint8_t strdigit_toInt(const char digit)
{

	uint8_t d;
	if((digit < '0' || digit > '9'))
		return EXIT_CODE_GLOBAL_UNSUPPORTED;

	d = (uint8_t) (digit - '0');
			
	return d;
}

unsigned int digit_count(uint32_t value)
{
	if(value >= 10 && value <= 99) return 2;
	else if(value >= 100 && value <= 999) return 3;
	else if(value >= 1000 && value <= 9999) return 4;
	else if(value >= 10000 && value <= 99999) return 5;
	else if(value >= 100000 && value <= 999999) return 6;
	else if(value >= 1000000 && value <= 9999999) return 7;
	else if(value >= 10000000 && value <= 99999999) return 8;
	else if(value >= 100000000 && value <= 999999999) return 9;
	else if(value >= 1000000000) return 10;

	return 1;
}

unsigned int hex_digit_count(uint32_t value)
{
	if(value >= 0x100 && value <= 0xFFF) return 3;
	else if(value >= 0x01000 && value <= 0x0FFFF) return 4;
	else if(value >= 0x10000 && value <= 0xFFFFF) return 5;
	else if(value >= 0x100000 && value <= 0xFFFFFF) return 6;
	else if(value >= 0x1000000 && value <= 0xFFFFFFF) return 7;
	else if(value >= 0x10000000 && value <= MAX) return 8;

	return 2;
}

void memset(void *start, size_t size, unsigned char val)
{
	ASSERT(start);

	uint8_t *s = start;

	while(size--)
		s[size] = val;
}

void sleep(uint32_t timeIn_ms)
{
	uint32_t current, wait_for;
	current 	= timer_getCurrentTick();
	wait_for	= current + timeIn_ms;

	/* should *in theory* stop you from possibly having a 7 week sleep when the computer is already on for 7 weeks. 
	   if not, I'm sorry, but also: many thanks for using Vireo for that long! :) */
	if(current + timeIn_ms >= MAX)
		wait_for = timeIn_ms - (MAX - current);

	while((current = timer_getCurrentTick()) < wait_for);
}

/* returns success (0 = zero) when the flag(s) is/are enabled and fail (1 = one) when
         the flag(s) is/are not enabled.
*/
unsigned char flag_check(unsigned int flag, unsigned int to_check)
{
    if((flag & to_check) == to_check) return EXIT_CODE_GLOBAL_SUCCESS;
    return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

// returns first occurance of the thing to be found
uint32_t find_in_str(const char *o, const char *fnd)
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

// searches character
unsigned char strchr(char *str, char ch)
{
	uint32_t i = 0;

	while(str[++i] != '\0')
		if(str[i] == ch) 
			return EXIT_CODE_GLOBAL_SUCCESS;

	return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

void move_str_back(char *str, uint32_t move_by)
{
	uint32_t i = strlen(str) + 1, j = i + move_by - 1;
	
	for(; i; i--)
		str[j--] = str[i-1];
}

void memcpy(void *_dest, void *_src, size_t size)
{
	uint32_t i;
	
	char *dest = _dest;
	char *src = _src;

	for(i = 0; i < size; ++i)
		*(dest + i) = *(src + i);

}

static uint32_t str_find_val(const char *str)
{
	size_t length = strlen(str);
	uint32_t i = 0;

	for(; i < length; ++i)
		if(str[i] == '%') 
			break;

	return i;
}

void str_add_val(char *str, const char *format, uint32_t value)
{
	size_t length = strlen(format);
	uint32_t val_index = str_find_val(format);
	size_t val_len = 0;

	memcpy(str, (void *) (format), val_index);

	if(val_index >= length)
		return;

	switch(format[val_index + 1])
	{
		case 'x':
		{
			char *s = hexstr(value, 0);
			val_len = strlen(s);
			memcpy(&str[val_index], s, val_len);		
			break;
		}

		case 'i':
		{
			char *s = intstr(value);
			val_len = strlen(s);
			memcpy(&str[val_index], s, val_len);
			break;
		}

		case 's':
			val_len = strlen((const char *) value);
			memcpy(&str[val_index], (char *) value, val_len);	
		break;
		
		case 'c':
			val_len = 1;
			str[val_index] = (char) value;
		break;	
	}

	val_len = val_len + val_index;

	memcpy(&str[val_len], (void *) &format[val_index + 2], strlen(&format[val_index + 2]));
	str[strlen(&format[val_index + 2]) + val_len] = '\0';
}

uint8_t nth_bit(uint32_t dword, uint8_t size)
{
	size = size > 32 ? 32 : size;

	for(uint8_t i = 0; i < size; ++i)
		if(dword == (1U << i))
			return i;
	
	return (uint8_t) MAX;
}

// like strtok() but non-destructive to the original string
uint8_t str_get_part(char *part_out, const char *s, const char *delim, uint32_t *pindex)
{
	uint32_t strindex = 0;

	if(*(pindex) == MAX)
		return 0; // done

	for(uint32_t i = 0; i < *(pindex); ++i)
	{
		if((strindex = strindex + find_in_str(&s[strindex], delim)) == MAX)
			break;

		strindex = strindex + strlen(delim);
	}

	*(pindex) = *(pindex) + 1;
	
	if(strindex == MAX)
	{
		*(pindex) = MAX;
		return 0;
	}

	uint32_t next = find_in_str(&s[strindex], delim);

	if(next == MAX)
	{
		next = strlen(&s[strindex]);
		*(pindex) = MAX;
	}
	
	memcpy(part_out, (void *) (&s[strindex]), next);
	part_out[next] = '\0';

	return 1; // not done
}

/* LIB C stuff */
char *strtok(char *s, const char *delim)
{
	static char *holder;

	if (s)
		holder = s;

	do {
		s = strsep(&holder, delim);
	} while (s && !*s);

	return s;
}

char *strsep(char **stringp, const char *delim)
{
	char *s = *stringp;
	char *e;

	if (!s)
		return NULL;

	e = strpbrk(s, delim);
	if (e)
		*e++ = '\0';

	*stringp = e;
	return s;
}

char *strpbrk(const char *s, const char *accept)
{
	const char *ss = (const char*) (s + __strxspn(s, accept, 1));

	return *ss ? (char *)ss : NULL;
}

static size_t __strxspn(const char *s, const char *map, char parity)
{
	char matchmap[UCHAR_MAX + 1];
	size_t n = 0;

	/* Create bitmap */
	memset(matchmap, sizeof matchmap, 0);
	while (*map)
		matchmap[(unsigned char)*map++] = 1;

	/* Make sure the null character never matches */
	matchmap[0] = parity;

	/* Calculate span length */
	while (matchmap[(unsigned char)*s++] ^ parity)
		n++;

	return n;
}
/* END LIB C stuff */