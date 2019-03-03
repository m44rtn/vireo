#ifndef GDT_H
#define GDT_H

#include "../include/types.h"
#include "memory.h"

typedef struct gdt_desc
{
	uint16_t lim_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_hig;
} __attribute__((packed)) gdt_desc;

typedef struct gdt_ptr
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdt_ptr;


void GDT();
static void setGDT(gdt_desc* descr, uint32_t bas, uint32_t limit, uint8_t access, uint8_t granularity);
void Prep_TSS();
void TSS_update_stack();

extern void gdt_set32(gdt_ptr* hello);

extern void getESP();



#endif
