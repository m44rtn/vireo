#ifndef TYPES_H
#define TYPES_H

#define NULL 0
#define UCHAR_MAX 255

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t; 
typedef unsigned long long uint64_t; 
//typedef char* object;
typedef uint32_t size_t; //stole it from stddef.h

typedef enum{
    false,
    true
} bool;

typedef struct 
{
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;

    uint32_t esp;
    uint32_t ebp;

    uint32_t esi;
    uint32_t edi;

    uint32_t EFLAGS;
} __attribute__ ((packed)) tREGISTERS;

#endif

