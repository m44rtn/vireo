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

#include "elf.h"
#include "task.h"
#include "exec.h"

#include "../include/types.h"
#include "../include/exit_code.h"

#include "../screen/screen_basic.h"

#include "../memory/paging.h"

#include "../util/util.h"

#include "../dbg/debug.h"

#define ELF_IDENT_SIZE      16

#define ELF_TYPE_NONE       0
#define ELF_TYPE_EXEC       2

#define ELF_MACHINE_NONE    0
#define ELF_MACHINE_386     3 // x86

#define ELF_VERSION_NONE    0 // invalid
#define ELF_VERSION_CURRENT 1

#define ELF_MAGIC_NUM       0x7f
#define ELF_MAGIC_STR       "ELF"

#define ELF_CLASS_32        1
#define ELF_DATA_LSB        1

#define ELF_PTYPE_LOAD      1

typedef struct
{
    uint8_t elf_ident[ELF_IDENT_SIZE];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff; // program header offset
    uint32_t shoff; // section header offset
    uint32_t flags;
    uint16_t ehsize; // elf header size
    uint16_t phentsize; // program header entry size
    uint16_t phnum; // # of program headers
    uint16_t shentsize; // section header entry size
    uint16_t shnum;
    uint16_t shstrndx; // section header table header
} __attribute__((packed)) elf_header_t;

typedef struct
{
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t file_size;
    uint32_t memsize;
    uint32_t flags;
    uint32_t align;
} __attribute__((packed)) elf_program_t;

static uint8_t elf_check_file(elf_header_t *hdr)
{
    if(hdr->elf_ident[0] == ELF_MAGIC_NUM && 
        !strcmp_until((char *) &(hdr->elf_ident[1]), ELF_MAGIC_STR, strlen(ELF_MAGIC_STR)))
            return 1; // success
    
    return 0;
}

static uint8_t elf_check_compat(elf_header_t *hdr)
{
    if(hdr->elf_ident[4] == ELF_CLASS_32 && hdr->elf_ident[5] == ELF_DATA_LSB
        && hdr->elf_ident[5] == ELF_VERSION_CURRENT && hdr->machine == ELF_MACHINE_386)
            return 1; // compatible
    
    return 0; // incompatible
}

static uint8_t elf_check_errors(elf_header_t *hdr)
{
    if(!elf_check_file(hdr))
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;

    if(!elf_check_compat(hdr))
        return EXIT_CODE_GLOBAL_UNSUPPORTED;
        
    if(hdr->type != ELF_TYPE_EXEC)
        return EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}

static void *elf_load_binary(void *file, pid_t pid)
{
    const elf_header_t *hdr = (elf_header_t *) file;
    const elf_program_t *prog = (elf_program_t *) (((uint32_t)file) + (hdr->phoff));

    void *ptr = 0;

    for(uint32_t i = 0; i < (hdr->phnum); ++i)
    {
        if((prog[i].type) != ELF_PTYPE_LOAD)
            continue;
        
        // allocate page for program thing
        PAGE_REQ req = {
            .pid = pid,
            .attr = PAGE_REQ_ATTR_READ_WRITE,
            .size = prog[i].memsize
        };
        char *loc = valloc(&req);
        ptr = (!ptr) ? loc : ptr;

        memcpy(loc, (void *) (((uint32_t)file) + prog[i].offset), prog[i].file_size);
    }

    return ptr;
}

// returns relative entry address, memory pointer to parsed binary in ptr, error in _err
void *elf_parse_binary(void **ptr, pid_t pid, err_t *_err)
{
    elf_header_t *hdr = (elf_header_t *) *ptr;

    if(!(*ptr))
        return NULL;

    void *entry = (void *) hdr->entry;
    
    uint8_t err;
    if((err = elf_check_errors(hdr)))
        *_err = err;
    
    void *nptr = elf_load_binary(*ptr, pid);
    vfree(*ptr);
    
    *ptr = nptr;

    *_err = EXIT_CODE_GLOBAL_SUCCESS;
    return entry; 
}