
#ifndef __GLOBAL_FLAGS_H__
#define __GLOBAL_FLAGS_H__

/* GLOBAL_FLAGS definitions */
#define GLOBAL_FLAG_QUIET   1

unsigned char flag_check(unsigned int flag, unsigned int to_check); 

typedef struct SYSTEM_INFO
{
    unsigned int GLOBAL_FLAGS;
} SYSTEM_INFO;
SYSTEM_INFO SystemInfo;

#endif