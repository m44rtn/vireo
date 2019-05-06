#include "util.h"

uint8_t getBYTE(uint32_t address, uint8_t BYTE){ /*0 is 1st byte*/
	uint32_t shift = BYTE * 8;
	uint8_t result = (uint8_t) (address >> shift);
	return result;
}

uint16_t getWORD(uint32_t address, uint8_t WORD){ //0 is 1st word
	uint32_t shift = WORD * 16;
	uint16_t result = (uint16_t) (address >> shift);
	return result;
}

uint8_t eqlstr(char* str, char* strcom){
	uint8_t result = 1;
	uint8_t size = strlen(str);
	
	if(size != (strlen(strcom)) )
		result = 0;
	else
	{
		uint8_t i = 0;
		for(i; i <= size; i++)
			if(str[i] != strcom[i]) result = 0;
			
	}
	
	return result;
}

void sleep(uint32_t sec)
{
	uint32_t CurrentTicks;
	uint32_t Ticks = sec * 256;
	uint32_t StartTicks	= systeminfo.PITcount;

	do
	{
		CurrentTicks = systeminfo.PITcount;
	} while (CurrentTicks < (Ticks + StartTicks));
	
}

static uint32_t TimerTicks;
void timer_start()
{
	TimerTicks = systeminfo.PITcount;
}

uint32_t timer_end()
{
	TimerTicks = systeminfo.PITcount - TimerTicks;
	return TimerTicks;
}

char* movestr(char* str, uint32_t from)
{
	uint32_t len = strlen(str);
	uint32_t ofrom = from;

	for(int i = 0; from < len; i++)
	{
		*(&str[i]) = *(&str[from]);
		from++;
	} 

	from -= ofrom;

	for(from; from < len; from++)
		str[from] = '\0';
	return str;
}

bool hasStr(char * str_longest, char* str_shortest)
{
	bool result = true;
	uint8_t size = strlen(str_shortest);
	
	int i = 0;
	for(i; i < size; i++){
		if(str_longest[i] != str_shortest[i]) result = false;
		
	}
	
	return result;
}


char* hexstr(long val){
	long tempval = val;
	char* outputstr = "00000000\0";
	//char* ptr =  outputstr;
	char chrIndex;
	char* hexDig = "0123456789ABCDEF";
	
	for(int8_t loopcntr = 8; loopcntr > 0; loopcntr--){
		chrIndex = tempval & 0x0000000F;
		outputstr[loopcntr] = hexDig[0 + chrIndex];
		tempval = tempval >> 4;
	}
	return outputstr;
}

/*char* intstr(long val)
{
	long tempval = val;
	char* outputstr = /*malloc(11); "00000000\0";

	char chrIndex;
	char *Dig = "0123456789";

	for(uint8_t i = 0; i < 8; i++)
	{
		chrIndex = ( (uint8_t) tempval & 0xFF) | 0x30;
		trace("%i", chrIndex);
		outputstr[i] = Dig[0 + chrIndex];
		tempval >> 8;
	}

	return outputstr;
}*/

char *intstr(uint32_t val) {          
    uint32_t i, sign;
	char *str, *ret_str;
    if ((sign = val) < 0) val = -val;
    i = 0;
    do {
        str[i++] = val % 10 + '0';         
    } while ((val /= 10) > 0);

    if (sign < 0) str[i++] = '-';

	i--;

	uint32_t bi = i + 1;
	uint32_t j;
	for(j = 0; j < bi; j++)
	{
		ret_str[j] = str[i];
		i--;
	}

	ret_str[j] = '\0';

	return ret_str;
    /* TODO: implement "reverse" */
}

uint32_t strlen(char* s){
	uint32_t i = 0;
	while(s[i])
		i++;
	return i;

}

char* TransformUpLowUC(char* str, bool lORu)
{
	char* stra = str;
	int strl = strlen(str);
	for(int i = 0; i < strl; i++)
	{
		if(str[i] == ' ') continue; //if it's a space character ignore
		if(str[i] == '\n') continue;
		if(str[i] == '\t') continue;
		char charrr = (char) str[i];


		if(lORu) charrr += 0x20; //lowercase
		if(!lORu) charrr -= 0x20; //uppercase
		stra[i] = charrr;

	}
	return stra;
}


//chops the party into parts
char *PartyChop(char* str, const char* delim){
	char* splits = strtok(str, delim);
	if(splits == NULL) return NULL;

	while(splits != NULL){
		splits = strtok(NULL, delim);
	}
	return splits;
}


/* ===================================================================
 *              NEXT PART IS FROM MEINOS LIB (string.c)
 * 				  https://github.com/jgraef/meinOS
 * ===================================================================
 */

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
	const char *ss = s + __strxspn(s, accept, 1);

	return *ss ? (char *)ss : NULL;
}

static size_t __strxspn(const char *s, const char *map, int parity)
{
	char matchmap[UCHAR_MAX + 1];
	size_t n = 0;

	/* Create bitmap */
	kmemset(matchmap, 0, sizeof matchmap);
	while (*map)
		matchmap[(unsigned char)*map++] = 1;

	/* Make sure the null character never matches */
	matchmap[0] = parity;

	/* Calculate span length */
	while (matchmap[(unsigned char)*s++] ^ parity)
		n++;

	return n;
}


/* ===================================================================
 *              	END MEINOS LIB (string.c)
 * 				  https://github.com/jgraef/meinOS
 * ===================================================================
 */
