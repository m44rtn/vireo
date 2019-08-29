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

#include "gdt.h"

#include "../include/types.h"

#define GDT_SEGMENT_TYPE_DATA       0
#define GDT_SEGMENT_TYPE_CODE       1

#define GDT_PRIVILEGE_ISRING0       0
#define GDT_PRIVILEGE_ISRING3       3

typedef struct GDT_ENTRY
{
    uint16_t     limit_low;
    uint16_t     base_low;
    uint8_t      base_mid;
    uint8_t      access;
    uint8_t      flags_plus_limit;
    uint8_t      base_high;
} __attribute__ ((packed)) GDT_ENTRY; 


/* internal GDT_ACCESS */
typedef struct GDT_ACCESS_
{
    bool isPresent;
    bool isRing3;
    bool isCodeSegment;
    bool dataisWritable; /* data segment only */
    bool codeisReadable; /* code segment only */
    bool isAccessed;
} GDT_ACCESS_;

static uint8_t GDT_prepare_flags(GDT_FLAGS flags);

/* sets up the GDT */
void GDT_setup(GDT_ACCESS access, GDT_FLAGS flags)
{
    GDT_ACCESS_ internal_access;

    internal_access.codeisReadable = access.codeisReadable;
    internal_access.dataisWritable = access.dataisWritable;
    


    /* TODO */
}

static void GDT_entry(GDT_entry *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    flags = (flags & 0xf0) << 4;
}

/* creates a flag byte based on the GDT_FLAGS struct to make things easier
   for us. */
static uint8_t GDT_prepare_flags(GDT_FLAGS flags)
{
    uint8_t use32;
    uint8_t output_flag = 0;

    output_flag = (uint8_t) output_flag | ((flags.Align4k & 1) << 7);

    /* Because use32 should actually be default, I chose to give the user the option to use16
    by setting use16 to true (otherwise you give them the option to use32, which is the default option,
    so I figured it would be weird to specifically ask people if they want to use the default option), but because
    of how the flags work in the GDT I need to flip the value. The GDT actually asks you if you want to use32 (and use16 is therefore the actual default),
    to use32 you need to set bit 6, which is why we flip it here. I hope I explained that good enough */
    use32 = (flags.use16)? 0 : 1; 
    output_flag = (uint8_t) output_flag | ((use32 & 1) << 6);

    return output_flag;
}

/* creates an access byte based on the GDT_ACCESS_ struct to make things easier
   for us. */
static uint8_t GDT_prepare_access(GDT_ACCESS_ access, uint8_t segment_type)
{
    uint8_t output_access = 0;

    output_access = ouput_access | ((access.isPresent & 1) << 7);

    if(access.isRing3)
        output_access = output_access | (GDT_PRIVILEGE_ISRING3 << 5);
    else
        output_access = output_access | (GDT_PRIVILEGE_ISRING0 << 5);
    
    /* bit 4 is always one */
    output_access = output_access | (1 << 4);

    output_access = output_access | (segment_type << 3);

    /* by using if else, we make sure that we're always safe no matter what argument is given.
       the routine expects TYPE_DATA or TYPE_CODE (0 or 1), so by using if-else we make sure that if
       a different (non-existing/expected) type is given that a) code is not readable and b) data is
       not writable. maybe it's overkill, but at least we make sure nothing weird happens... :) */ 
    if(segment_type == GDT_SEGMENT_TYPE_DATA) 
        output_access = output_access | ((access.dataisWritable & 1) << 1);
    else if(segment_type == GDT_SEGMENT_TYPE_CODE)
        output_access = output_access | ((access.codeisReadable & 1) << 1);

    output_access = ouput_access | (access.isAccessed & 1);
    
    return output_access;
}