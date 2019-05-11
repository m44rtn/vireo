#ifndef MEMORY_H
#define MEMORY_H

#include "../include/types.h"
#include "../include/info.h"
#include "../include/GRUB/multiboot.h" //for the grub stuff
#include "../include/error.h"



void *malloc(size_t size);
void demalloc(void *ptr);

void memory_init(multiboot_info_t* mbh);
void GRUB_GetMemInfo(multiboot_info_t* mbh); 

void *kmemset(void *ptr, uint32_t val, size_t size);

void MemCopy(uint32_t *src, uint32_t *dest, uint32_t size);
char *strcopy(char* dest, char* src);
void strncpy(char *dest, char* src, uint32_t size);

extern void memory_set_stack();

//#define KMEMSET_KOTHER 0xB17D05 //BirdOS

#endif
