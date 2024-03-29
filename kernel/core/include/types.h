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

#ifndef __TYPES_H__
#define __TYPES_H__

#define NULL    0

#define FALSE   0
#define TRUE    1

/* if you want quiet start-up: */
#define NO_DEBUG_INFO

/* if you don't want assertions (dbg.h): */
/*#define NDEBUG*/

#define UCHAR_MAX 255

#define MAX       0xFFFFFFFF

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef uint8_t size8_t;
typedef uint32_t size_t; 

typedef uint16_t syscall_t;
typedef uint16_t api_space_t;
typedef uint32_t api_key_t;
typedef uint8_t err_t;

typedef uint32_t return_t; // return addresses

typedef uint8_t pid_t; // process id

typedef uint8_t bool_t;

typedef void file_t;

typedef enum bool {
    false,
    true
} bool;

#endif