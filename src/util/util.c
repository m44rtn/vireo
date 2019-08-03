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

char* hexstr(unsigned int val)
{
	unsigned int tempval = val;
	char* outputstr = "00000000\0";
	
	char chrIndex;
	char* hexDig = "0123456789ABCDEF";
	
	for(int8_t loopcntr = 8; loopcntr > 0; loopcntr--){
		chrIndex = tempval & 0x0000000F;
		outputstr[loopcntr] = hexDig[0 + chrIndex];
		tempval = tempval >> 4;
	}
	return outputstr;
}

/* TODO: make nicer */
char *intstr(uint32_t val) 
{          
    uint32_t str_loc = 0, sign = 0;
	
    char *str, *ret_str;
    
    if ((sign = val) < 0) val = -val;;
    
    do {
        str[str_loc++] = val % 10 + '0';         
    } while ((val /= 10) > 0);

    if (sign < 0) str[str_loc++] = '-';

    str_loc--;

	uint32_t EndOfString = str_loc + 1;
	
    uint32_t i;
	for(i = 0; j < EndOfString; i++)
	{
		ret_str[j] = str[i];
		str_loc--;
	}

	ret_str[j] = '\0';

	return ret_str;
}