#ifndef CPU_H
#define CPU_H

#include "../include/types.h"
#include "screen.h"
#include "../io/sys.h"
#include "../include/info.h"


typedef struct idt_desc{
	uint16_t offset1;
	uint16_t selector;
	uint8_t zero;
	uint8_t typeatrib;
	uint16_t offset2;
	
} __attribute__((packed)) idt_desc;

typedef struct idt_ptr{
	uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr;


void idtent();
void setidt(int n, uint32_t handler);
void cpustuff();
void IDT_add(int n, uint32_t handler);
static void mainasm(int num);

void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi);

void CPUID();
extern void GetCPUVendor();
#endif
