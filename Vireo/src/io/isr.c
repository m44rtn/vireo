#include "isr.h"


#define PIC1DATA  (PIC1 + 1)
#define PIC2DATA  (PIC2 + 1)



char tckcounter;
uint16_t sec = 0;

void isrinst(){
	//types:
	//0b0101 	0x5 	5 	80386 32 bit task gate
	//0b0110 	0x6 	6 	80286 16-bit interrupt gate
	//0b0111 	0x7 	7 	80286 16-bit trap gate
	//0b1110 	0xE 	14 	80386 32-bit interrupt gate
	//0b1111 	0xF 	15 	80386 32-bit trap gate
	//selector = 0x08
	//Selector 	bits 16..31 	description: Selector 0..15 
	
	setidt(0x00, (uint32_t)isr0);
	setidt(0x01, (uint32_t)isr1);
	setidt(0x03, (uint32_t)isr3);
	setidt(0x04, (uint32_t)isr4);
	setidt(0x07, (uint32_t)isr7);
	setidt(0x08, (uint32_t)isr8);
	setidt(0x10, (uint32_t)isr10);
	setidt(0x11, (uint32_t)isr11);
	setidt(0x0D, (uint32_t)isr13);
	setidt(0x0E, (uint32_t)isr14); //Page Fault
	setidt(0x15, (uint32_t)isr15);
	setidt(0x20, (uint32_t)isr20);
	setidt(0x21, (uint32_t)isr21); 

	setidt(0x30, (uint32_t)isr30);
	setidt(0x47, (uint32_t)isr47);
	
	idtent();
}

void isr0c(){ 

	//divide by 0 exception
	kernel_panic_dump("DEVISION_BY_ZERO");
	outb(PIC1, 0x20); //say the PIC that we're done with the interrupt

}

void isr1c(){
	kernel_panic_dump("DEBUG");
	sleep(300);
	outb(PIC1, 0x20);
	
}

void isr3c(uint32_t eax){
	switch(eax){
		case 0x01:		//return to ring 0
			break;
	}
	outb(PIC1, 0x20);
}

void isr4c(){
	asm("int $15");
}

void isr7c(){
	outb(PIC1, 0x0B);
	uint8_t irr = inb(0x20);
	if(irr & 0x80 == 1){
		
	}else{
	outb(PIC1, 0x20); //say the PIC that we're done with the interrupt
	}
	asm("iret");
}

void isr8c(){

	kernel_panic_dump("DOUBLE_FAULT");
	while(1);
	outb(PIC1,0x20);
	outb(PIC2, 0x20);
}

void isr11c(){
	kernel_panic("AGNMENT_CHECK");
}

void isr12c(){
		kernel_panic("STACK_SEGMENT_FAULT");
	asm("hlt");
}

typedef struct
{
	uint32_t ss;
	uint32_t esp;
	//uint32_t eflags;
	uint32_t cs;
	uint32_t eip;

	uint32_t ax;
	uint32_t di;
} __attribute__ ((packed)) CTX;

uint8_t last_interrupt;
uint32_t *int_stack;

uint16_t before_int_ax, before_int_di;

void isr13c(uint16_t ip, uint16_t cs, uint16_t esp, uint16_t ss)
{ 
	//general protection fault

	
	//trace("\t-EFLAGS=%i\n", eflags);
	CTX ctx;
	
	uint16_t *stack = (uint16_t *) v86_sgoff_to_linear(ss, esp);
	uint16_t *ivt = 0; //255 IVT entries
	uint8_t *ip_addr = (uint8_t *) v86_sgoff_to_linear(cs, ip);
	uint16_t ax, di;
	
	switch(ip_addr[0])
	{
		case 0xcd:
			//print("v86 INTERRUPT\n");

			//reserve some stack space
			stack -= 3;
			ctx.esp = ((esp & 0xffff) - 8) & 0xffff;

			//put our return stuff on the stack
			stack[0] =  (uint16_t) ip_addr + 2;
			stack[1] = 0x1b;
			stack[2] = (uint16_t) 0x20202;

			int_stack = (uint32_t *) ctx.esp;

			ctx.cs =  ivt[ *(ip_addr + 1) * 2 + 1];
			ctx.ss =  (uint32_t) 0x23;
			ctx.eip = (uint32_t) ivt[*(ip_addr + 1) * 2];
			
			
			//trace("tasks[0].registers.eax = %i\n", tasks[0].registers.eax);
			//trace("tasks[0].registers.edi = %i\n", tasks[0].registers.edi);
			
			//trace("stack = %i\n",    (uint32_t) stack);
			//trace("stack[0] = %i\n", (uint32_t) stack[0]);

			/*trace("CTX: -IP=%i", ctx.eip);
			trace("\t-CS=%i", ctx.cs);
			trace("\t-SP=%i", ctx.esp);
			trace("\t-SS=%i\n", ctx.ss);*/

			last_interrupt = *(ip_addr + 1);

			outb(PIC1, 0x20);
			v86_enter((uint32_t *) &ctx, (uint32_t *) &tasks[0].registers);
		break; 

		case 0x9c:
			//print("v86 PUSHF\n");
			//stack -= 1;
			ctx.esp = ((esp & 0xffff) - 2) & 0xffff;

			//maybe this needs to be switched around
			stack[0] = (uint16_t) 0; //0x200006;
						
			ctx.cs = (uint32_t) cs;
			ctx.ss = 0x23;
			ctx.eip = (uint32_t) ip + 1;

			outb(PIC1, 0x20);
			v86_enter((uint32_t *) &ctx, (uint32_t *) &tasks[0].registers);
		break;

		case 0x9d:
			//basically just ignore this one
			//print("v86 POPF\n");
			ctx.esp = ((esp & 0xffff) + 2) & 0xffff;
			//ctx.eflags = (uint16_t) 0x20202;

			stack[0] = 0;

			ctx.cs = (uint32_t) cs;
			ctx.ss = 0x23;
			ctx.eip = (uint32_t) ip + 1;

			outb(PIC1, 0x20);
			v86_enter((uint32_t *) &ctx, (uint32_t *) &tasks[0].registers);
		break;

		case 0xcf:
			//print("v86 IRET\n");

			trace("stack = %i\n",    (uint32_t) stack);
			trace("stack[0] = %i\n", (uint32_t) stack[1]);
						
			ctx.eip	= (uint32_t) (stack[1] - 0x1b0);
			ctx.cs	=  0x1b;
			ctx.esp	= (uint32_t) ( (esp & 0xffff) + 8) & 0xffff;
			ctx.ss	=  0x23;

			
			/*trace("CTX: -IP=%i", ctx.eip);
			trace("\t-CS=%i", ctx.cs);
			trace("\t-SP=%i", ctx.esp);
			trace("\t-SS=%i\n", ctx.ss);*/
										
			outb(PIC1, 0x20);
			v86_enter((uint32_t *) &ctx, (uint32_t *) &tasks[0].registers);
		break;

		case 0xF0:
			print("v86 LOCK\n");
		break;

		case 0xFA:
			//ignore
			print("v86 CLI\n");

			ctx.eip	= (uint32_t) ip + 1;
			ctx.cs	= (uint32_t) cs;
			ctx.esp	= (uint32_t) esp;
			ctx.ss	= (uint32_t) ss;

			outb(PIC1, 0x20);
			v86_enter((uint32_t *) &ctx, (uint32_t *) &tasks[0].registers);
		break;

		case 0xFB:
			//ignore
			print("v86 STI\n");
			ctx.eip	= (uint32_t) ip + 1;
			ctx.cs	= (uint32_t) cs;
			ctx.esp	= (uint32_t) esp;
			ctx.ss	= (uint32_t) ss;
				
			outb(PIC1, 0x20);
			v86_enter((uint32_t *) &ctx, (uint32_t *) &tasks[0].registers);
		break;

		default:
			kernel_panic_dump("GENERAL_PROTECTION_FAULT");
			while(1); //just for testing purposes
			kernel_panic("GENERAL_PROTECTION_FAULT");
			
			while(1);
		break;

	}
	outb(PIC1, 0x20);
	
}
void isr14c(){
	kernel_panic("PAGE_FAULT");
}
void isr15(){
	kernel_panic("UNSUPPORTED_INTERRUPT");
	asm("hlt");
	
}
uint16_t counter;
void isr20c(){
	systeminfo.PITcount++;
	
	if(systeminfo.PITcount > 0xFFFFFFFF) systeminfo.PITcount == 0;

	if((systeminfo.FLAGS & INFO_FLAG_MULTITASKING_ENABLED))
		task_timeguard();

	outb(PIC1, 0x20);
}

void isr21c(){
	int c = inb(0x60);

	if(c < 0x81){
		keybin((char) c);
	}
	outb(PIC1,0x20);
}


void isr30c(){

	
	outb(PIC1,0x20);
	outb(PIC2,0x20);
	
}
void isr47c(){
	print("ISR 47 at your service!\n");
	outb(PIC1,0x20);
	outb(PIC2,0x20);
}
