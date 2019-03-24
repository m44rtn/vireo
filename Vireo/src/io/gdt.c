#include "gdt.h"

#define gdtlen 7

typedef struct{
    uint32_t prev;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;

    uint32_t CR3;
    uint32_t eip;
    uint32_t EFLAGS;

    uint32_t EAX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t EBX;
    uint32_t ESP;
    uint32_t EBP;
    uint32_t ESI;
    uint32_t EDI;

    uint32_t ES;
    uint32_t CS;
    uint32_t SS;
    uint32_t DS;
    uint32_t FS;
    uint32_t GS;
    uint32_t LDTR;
    uint32_t IOPB;
} __attribute__ ((packed)) TSS;

extern int StackPointer;
static gdt_desc gdt[gdtlen];
static gdt_ptr gdtptr;
TSS tss;


void Prep_TSS()
{
	kmemset(&tss, NULL, sizeof(TSS));
	tss.ss0 = 0x10; //0x10;

	getESP();
	tss.esp0 = StackPointer;

    tss.ES = 0x10;
    tss.CS = 0x10;
    tss.DS = 0x10;
    tss.FS = 0x10;
    tss.GS = 0x10;

    tss.LDTR = &gdtptr;

    tss.IOPB = sizeof(TSS);
}

void TSS_update_stack()
{
	getESP();
	tss.esp0 = StackPointer;
}

void GDT(){

	setGDT(&gdt[0], 0, 0, 0, 0); 						//NULL
	setGDT(&gdt[1], 0, 0xFFFFFFFF, 0x9A, 0xCF); 		    //ring0 code
	setGDT(&gdt[2], 0, 0xFFFFFFFF, 0x92, 0xCF);          //ring0 data
	
	//setGDT(&gdt[3], 0x7FFEFFF, 0x7FFFFFF, 0x92, 0xCF);  //ring0 data, stack, 4096 bytes (4kib)
	
	setGDT(&gdt[3], 0, 0xFFFFFFFF, 0xFA, 0xCF); //ring3 code
	setGDT(&gdt[4], 0, 0xFFFFFFFF, 0xF2, 0xCF); //ring3 data

	setGDT(&gdt[5], &tss, (&tss + sizeof(TSS)), 0x89, 0x40/*0xCF*/);
	
	gdtptr.base = (uint32_t) &gdt;						//It's actually just the pointer to the GDT
	gdtptr.limit = sizeof(gdt_desc) * gdtlen - 1;		//And it's size
	gdt_set32(&gdtptr);
	
}


static void setGDT(gdt_desc* descr, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity){
	
	descr -> base_low = (base & 0xffff);		//low
	descr -> base_mid = (base >> 16) & 0xff;    //middle
	descr -> base_hig = (base >> 24) & 0xff;    //high
	descr -> lim_low = (limit & 0xffff);		//limit
	descr -> granularity = ((limit >> 16) & 0x0f) | (granularity & 0xf0); 
	descr -> access = access;	
}



//stuff to know:
// 0x00 = NULL descriptor
// 0x9A = code descriptor
// 0x92 = data descriptor
// 0x89 = TSS
