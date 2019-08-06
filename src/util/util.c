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


unsigned int strlen(char *str)
{
    uint32_t i = 0;
    while(str[i])
        i++;

    return i;
}

/* FIXME: malloc */
char* hexstr(unsigned int value)
{
	unsigned int tempval = value;
	char* outputstr = (char *) "00000000";
	
	char chrIndex;
	const char* hexDig = "0123456789ABCDEF";
	
	int8_t loopcntr;
	for(loopcntr = 7; loopcntr >= 0; loopcntr--){
		chrIndex = tempval & 0x0000000F;
		outputstr[loopcntr] = (char) hexDig[0 + chrIndex];
		tempval = tempval >> 4;
	}
	return outputstr;
}

/* FIXME: malloc */
/* Will only do unsigned integers :) */
char *intstr(uint32_t value) 
{          
    char *ReturnString = 0xFFFF;
	int strloc = (int) digit_count(value);
	uint32_t NullChar = digit_count(value);
	

	for(strloc = strloc - 1; strloc >= 0; strloc--)
	{
		ReturnString[strloc] = value % 10 + '0';
		value = value / 10;
	}	

	ReturnString[NullChar] = '\0';

	return ReturnString;
}

unsigned int digit_count(uint32_t value)
{
	/* So, the if statements are there for performance, because I've heard 
	devision takes a lot of time.  */

	if(value > 0 && value <= 9) return 1;
	else if(value >= 10 && value <= 99) return 2;
	else if(value >= 100 && value <= 999) return 3;
	else if(value >= 1000 && value <= 9999) return 4;
	else if(value >= 10000 && value <= 99999) return 5;
	else if(value >= 100000 && value <= 999999) return 6;
	else if(value >= 1000000 && value <= 9999999) return 7;
	else if(value >= 10000000 && value <= 99999999) return 8;
	else if(value >= 100000000 && value <= 999999999) return 9;
	else if(value >= 1000000000) return 10; /* we can never have more than 10 digits in 32-bit PM */

	/* in this case the value was 0, and so it is one digit.
	(done to avoid compiler warning) */
	return 1;
}