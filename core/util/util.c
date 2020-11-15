/*
MIT license
Copyright (c) 2019 Maarten Vermeulen

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

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../hardware/timer.h"

#include "../screen/screen_basic.h"

char utilPool[32];

static size_t __strxspn(const char *s, const char *map, char parity);

unsigned int strlen(char *str)
{
    uint32_t i = 0;
    while(str[i])
        ++i;

    return i;
}

unsigned char strcmp(char *str1, char *str2)
{
	uint32_t i = 0;

	while(str1[i] && str2[i])
	{
		if(str1[i] != str2[i])
			return EXIT_CODE_GLOBAL_GENERAL_FAIL;
		++i;
	}

	return EXIT_CODE_GLOBAL_SUCCESS;
}

/* digit_amount: amount of digits to show, if 0 (or the value is bigger) the normal 
 amount of digits is used. meaning enough to show the actual value.*/
char* hexstr(unsigned int value, uint8_t digit_amount)
{
	unsigned int tempval = value;
	char *outputstr = (char *) &utilPool;
	int8_t i;
	uint32_t digits = (!digit_amount || digit_amount < hex_digit_count(value))? hex_digit_count(value) : digit_amount;

	uint8_t chrIndex;
	const char* hexDig = "0123456789ABCDEF";

	for(i = (int8_t) (digits - 1); i >= 0; i--){
		chrIndex = tempval & 0x0000000F;
		
		outputstr[i] = hexDig[chrIndex];
		tempval = tempval >> 4;
	}

	outputstr[digits] = '\0';
	
	return outputstr;
}

char *intstr(uint32_t value) 
{          
	char *ReturnString = (char *) &utilPool;
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
	else if(value >= 0x10000000 && value <= 0xFFFFFFFF) return 8;

	return 2;
}

void memset(char *start, size_t size, char val)
{
	while(size--)
		start[size] = val;
}

void sleep(uint32_t timeIn_ms)
{
	uint32_t current, wait_for;
	current 	= timer_getCurrentTick();
	wait_for	= current + timeIn_ms;

	/* should *in theory* stop you from possibly having a 7 week sleep when the computer is already on for 7 weeks. 
	   if not, I'm sorry, but also: many thanks for using Vireo for that long! :) */
	if(current + timeIn_ms >= 0xFFFFFFFF)
		wait_for = timeIn_ms - (0xFFFFFFFF - current);

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

unsigned char strchr(char *str, char ch)
{
	uint32_t i = 0;

	while(str[++i] != '\0')
		if(str[i] == ch) 
			return EXIT_CODE_GLOBAL_SUCCESS;

	return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

void memcpy(char *destination, char *source, size_t size)
{
	uint32_t i;
	
	for(i = 0; i < size; ++i)
		*(destination + i) = *(source + i);
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