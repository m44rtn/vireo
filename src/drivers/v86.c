//Handles the 'monitoring' of the virtual 8086 environment


#include "v86.h"

///

typedef struct
{
	uint32_t ss;
	uint32_t esp;
	uint32_t cs;
	uint32_t eip;
} __attribute__ ((packed)) CTX;

uint32_t v86_linear_to_sgoff(uint32_t ptr)
{
    
    uint16_t segment, offset;
    //offset + 16*segment

    offset = ptr & 0xf;
    segment = (ptr - offset) / 16;
    return ((uint32_t) segment << 16 | offset);
}

/* not necessarry, so maybe just use this as logic
 * or is it?*/
uint32_t v86_sgoff_to_linear(uint16_t segment, uint16_t offset)
{
    uint32_t LinearAddress = (segment * 16) + offset;
    return LinearAddress;
}

void v86_interrupt(uint16_t interrupt, tREGISTERS *registers)
{
    /* the location of the ivt */
    uint16_t *ivt   =   0x0000;
    CTX ctx;
    ctx.cs          =   *(&ivt[ interrupt * 2 + 1]);
	ctx.ss          =   (uint32_t) 0x23;
	ctx.eip         =   (uint32_t) *(&ivt[interrupt * 2]);
    ctx.esp         =   0xFFFF;

    trace("ctx.cs  = %i\n", ctx.cs);
    trace("ctx.esp = %i\n", ctx.esp);
    trace("ctx.eip = %i\n", ctx.eip);
    trace("loc_registers = %i\n", (uint32_t) registers);

    //while(1);
    v86_enter(&ctx, registers);			
}

void v86_save_state(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, 
                    uint32_t ecx, uint32_t eax,  uint32_t eip, uint32_t cs)
{
    //save all registers
    tREGISTERS *registers = (tREGISTERS *) 0x4000;
   
    registers->eax = eax;
    registers->ecx = ecx;
    registers->edx = edx;
    registers->ebx = ebx;

    registers->ebp = ebp;
    registers->esp = esp;

    registers->esi = esi;
}



