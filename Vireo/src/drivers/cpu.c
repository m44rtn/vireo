#include "cpu.h"

#define CPUID_OLDAMD "AMDisbetter!"
#define CPUID_AMD "AuthenticAMD"
#define CPUID_INTEL "GenuineIntel"
#define CPUID_VMWARE "VMwareVMware"

#define idtlen 256
#define cpuvendorid 0x00

static idt_desc idt[idtlen];
static idt_ptr idtptr;

void idtent(){
	idtptr.base = (uint32_t) &idt;
	idtptr.limit = idtlen * sizeof(idt_desc) - 1;
	
	//idt_set(&idtptr);
	__asm__ __volatile__("lidtl (%0)" : : "r" (&idtptr)); //register the idt to the cpu
	
	
}

void setidt(int n, uint32_t handler){
	idt[n].offset1 = (uint16_t) (handler & 0x0000ffff); //low
	idt[n].selector = 0x08; 
	idt[n].zero = 0;
	idt[n].typeatrib = 0x8E;
	idt[n].offset2 = (uint16_t) ((handler >> 16) & 0x0000ffff); //high
}

char* cpuvend;
char* VendorID;

void CPUID(){
	
	GetCPUVendor();
	systeminfo.CPUVendor = (char*) VendorID;
	
}	

void IDT_add(int n, uint32_t handler)
{
	idt[n].offset1 = (uint16_t) (handler & 0x0000ffff); //low
	idt[n].selector = 0x08; 
	idt[n].zero = 0;
	idt[n].typeatrib = 0x8E;
	idt[n].offset2 = (uint16_t) ((handler >> 16) & 0x0000ffff); //high

	asm("cli");

	idtent();

	asm("sti");
}


// let's do the msr stuff here

void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
	asm volatile("wrmsr" :: "a" (lo), "d" (hi), "c" (msr));
}

