#ifndef MEMORY_H
#define MEMORY_H

#include "../include/types.h"
#include "../include/info.h"
#include "../include/GRUB/multiboot.h" //for the grub stuff
#include "../include/error.h"



void *malloc(size_t size);
void demalloc(void *ptr);


void GRUB_GetMemInfo(multiboot_info_t* mbh); 

void *kmemset(void *ptr, uint32_t val, size_t size);

void MemCopy(uint32_t *src, uint32_t *dest, size_t size);
char *strcopy(char* dest, char* src);

//#define KMEMSET_KOTHER 0xB17D05 //BirdOS

#endif
