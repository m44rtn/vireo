#ifndef UTIL_H
#define UTIL_H

#include "../include/types.h"
#include "sys.h"
#include "../drivers/screen.h"
#include "memory.h"

#ifndef UTIL_LOWERCASE
#define UTIL_LOWERCASE true
#endif

#ifndef UTIL_UPPERCASE
#define UTIL_UPPERCASE false
#endif

uint8_t getBYTE(uint32_t address, uint8_t BYTE);
uint16_t getWORD(uint32_t address, uint8_t BYTE);
char* hexstr(long val);
uint8_t eqlstr(char* str, char* strcom);

void sleep(uint32_t sec);

// never used (optimize)
void timer_start();
uint32_t timer_end();


char *movestr(char* str, uint32_t from);
char* TransformUpLowUC(char* str, bool lORu);
uint32_t strlen(char* s);

char *PartyChop(char* str, const char* delim);


bool hasStr(char * str, char* str2);

//MEINOS LIB (string.c) -> https://github.com/jgraef/meinOS
char *strtok(char *s, const char *delim);
char *strsep(char **stringp, const char *delim);
char *strpbrk(const char *s, const char *accept);
static size_t __strxspn(const char *s, const char *map, int parity);
//end of MEINOS LIB (string.c) -> https://github.com/jgraef/meinOS

#endif
