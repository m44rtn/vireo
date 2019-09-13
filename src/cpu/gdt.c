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

#include "../screen/screen_basic.h"

#define GDT_LENGTH                  5

#define GDT_SEGMENT_TYPE_DATA       0
#define GDT_SEGMENT_TYPE_CODE       1

#define GDT_PRIVILEGE_ISRING0       0
#define GDT_PRIVILEGE_ISRING3       3

typedef struct GDT_DESC
{
    uint16_t size;
    uint32_t offset; /* aka linear address */
} __attribute__ ((packed)) GDT_DESC;

typedef struct GDT_ENTRY
{
    uint8_t      limit_low;
    uint8_t      limit_mid;
    uint8_t      base_low;
    uint8_t      base_mid1;
    uint8_t      base_mid2;
    uint8_t      access;
    uint8_t      flags_plus_limit;
    uint8_t      base_high;
} __attribute__ ((packed)) GDT_ENTRY; 


/* internal GDT_ACCESS */
typedef struct GDT_ACCESS_
{
    bool isPresent;
    bool isRing3;
    bool dataisWritable; /* data segment only */
    bool codeisReadable; /* code segment only */
    bool isAccessed;
} GDT_ACCESS_;

static void GDT_entry(GDT_ENTRY *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
static uint8_t GDT_prepare_flags(GDT_FLAGS flags);
static uint8_t GDT_prepare_access(GDT_ACCESS_ access, uint8_t segment_type);

/* sets up the GDT */
void GDT_setup(GDT_ACCESS access, GDT_FLAGS flags)
{
    GDT_ACCESS_ internal_access;
    GDT_ENTRY GDT[GDT_LENGTH] __attribute__((aligned(4096)));
    GDT_DESC  descriptor;

    uint8_t varFlags;

    internal_access.codeisReadable = access.codeisReadable;
    internal_access.dataisWritable = access.dataisWritable;
    
    GDT_entry(&GDT[0], 0, 0, 0, 0);

    internal_access.isPresent   = true;
    internal_access.isRing3     = false;
    internal_access.isAccessed  = false;

    varFlags  = GDT_prepare_flags(flags); 
        
    /* Ring 0 stuff */
    GDT_entry(&GDT[1], 0, 0x000FFFFF, GDT_prepare_access(internal_access, GDT_SEGMENT_TYPE_CODE), varFlags);
    GDT_entry(&GDT[2], 0, 0x000FFFFF, GDT_prepare_access(internal_access, GDT_SEGMENT_TYPE_DATA), varFlags);

    internal_access.isRing3     = true;

    /* Ring 3 stuff */
    GDT_entry(&GDT[3], 0, 0x000FFFFF, GDT_prepare_access(internal_access, GDT_SEGMENT_TYPE_CODE), varFlags);
    GDT_entry(&GDT[4], 0, 0x000FFFFF, GDT_prepare_access(internal_access, GDT_SEGMENT_TYPE_DATA), varFlags);

    descriptor.offset = (uint32_t) &GDT[0];
    descriptor.size   = (sizeof(GDT_ENTRY) * GDT_LENGTH) - 1;

    /* FIXME: remove traces */
    trace("GDT: %x\t", descriptor.offset);
    trace("GDT size: %x\t", descriptor.size);
    trace("entry size: %x\n", sizeof(GDT_ENTRY));
    
    /* while(2); */
    ASM_GDT_SUBMIT((uint32_t *) &descriptor);

    /* TODO: TSS 'n such */
}

static void GDT_entry(GDT_ENTRY *entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    entry->limit_low        = (uint8_t) (limit & 0xFF);
    entry->limit_mid        = (uint8_t) ((limit >> 8) & 0xFF);
    entry->base_low         = (uint8_t) (base & 0xFF);
    entry->base_mid1        = (uint8_t) ((base >> 8) & 0xFF);
    entry->base_mid2        = (uint8_t) ((base >> 16) & 0xFF);
    entry->access           = access;
    entry->flags_plus_limit = (uint8_t) (flags | ((limit >> 16) & 0x0F));
    entry->base_high        = (uint8_t) ((base >> 24) & 0xFF);

}

/* creates a flag byte based on the GDT_FLAGS struct to make things easier
   for us. */
static uint8_t GDT_prepare_flags(GDT_FLAGS flags)
{
    uint8_t use32;
    uint8_t output_flag = 0;

    output_flag = (uint8_t) (output_flag | ((flags.Align4k & 1) << 7));

    /* Because use32 should actually be default, I chose to give the user the option to use16
    by setting use16 to true (otherwise you give them the option to use32, which is the default option,
    so I figured it would be weird to specifically ask people if they want to use the default option), but because
    of how the flags work in the GDT I need to flip the value. The GDT actually asks you if you want to use32 (and use16 is therefore the actual default),
    to use32 you need to set bit 6, which is why we flip it here. I hope I explained that good enough */
    use32 = (flags.use16)? 0 : 1; 
    output_flag = (uint8_t) (output_flag | ((use32 & 1) << 6));

    return output_flag;
}

/* creates an access byte based on the GDT_ACCESS_ struct to make things easier
   for us. */
static uint8_t GDT_prepare_access(GDT_ACCESS_ access, uint8_t segment_type)
{
    uint8_t output_access = 0;

    output_access = (uint8_t) (output_access | ((access.isPresent & 1) << 7));

    if(access.isRing3)
        output_access = output_access | (GDT_PRIVILEGE_ISRING3 << 5);
    else
        output_access = output_access | (GDT_PRIVILEGE_ISRING0 << 5);
    
    /* bit 4 is always one */
    output_access = output_access | (1 << 4);

    output_access = (uint8_t) (output_access | (segment_type << 3));

    if(segment_type == GDT_SEGMENT_TYPE_DATA) 
        output_access = (uint8_t) (output_access | ((access.dataisWritable & 1) << 1));
    else if(segment_type == GDT_SEGMENT_TYPE_CODE)
        output_access = (uint8_t) (output_access | ((access.codeisReadable & 1) << 1));

    output_access = (uint8_t) (output_access | (access.isAccessed & 1));
    
    return output_access;
}