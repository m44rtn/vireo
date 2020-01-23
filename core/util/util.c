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

#include "../hardware/timer.h"

unsigned int strlen(char *str)
{
    uint32_t i = 0;
    while(str[i])
        i++;

    return i;
}

/* digit_amount: amount of digits to show, if 0 (or the value is bigger) the normal 
 amount of digits is used. meaning enough to show the actual value.*/
char* hexstr(unsigned int value, uint8_t digit_amount)
{
	unsigned int tempval = value;
	char* outputstr = (char *) 0xFFFF; /*"00000000";*/
	uint32_t digits = (!digit_amount || digit_amount < hex_digit_count(value))? hex_digit_count(value) : digit_amount;

	char chrIndex;
	const char* hexDig = "0123456789ABCDEF";

	int8_t loopcntr;
	for(loopcntr = (int8_t) (digits - 1); loopcntr >= 0; loopcntr--){
		chrIndex = tempval & 0x0000000F;
		outputstr[loopcntr] = (char) hexDig[0 + chrIndex];
		tempval = tempval >> 4;
	}

	outputstr[digits] = '\0';
	
	return outputstr;
}

/* FIXME: malloc */
char *intstr(uint32_t value) 
{          
    char *ReturnString = (char *) 0xFFFF;
	int strloc = (int) digit_count(value);
	uint32_t NullChar = digit_count(value);
	

	for(strloc = strloc - 1; strloc >= 0; strloc--)
	{
		ReturnString[strloc] = (char) (value % 10 + '0');
		value = value / 10;
	}	

	ReturnString[NullChar] = '\0';

	return ReturnString;
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
	current = timer_getCurrentTick();
	wait_for	 = current + timeIn_ms;

	/* should *in theory* stop you from possibly having a 7 week sleep when the computer is already on for 7 weeks. 
	   if not, I'm sorry, but also: many thanks for using Vireo for that long! :) */
	if(current + timeIn_ms >= 0xFFFFFFFF)
		wait_for = timeIn_ms - (0xFFFFFFFF - current);

	while( (current = timer_getCurrentTick()) != wait_for);
}