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
	kernel_panic("DEVISION_BY_ZERO");
	outb(PIC1, 0x20); //say the PIC that we're done with the interrupt

}

void isr1c(){
	kernel_panic("DEBUG");
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

	kernel_panic("DOUBLE_FAULT");
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

void isr13c(uint32_t ip, uint32_t cs/*, uint32_t sp, uint32_t ss*/){ //general protection fault

	setcolor(0x0E);
	print("\n\n#GP");
	setcolor(0x07);
	trace(" -IP=%i", ip);
	trace("\t-CS=%i\n", cs);
	//trace("\t-SP=%i", sp);
	//trace("\t-SS=%i\n", ss);
	
	uint8_t *ip_addr = v86_sgoff_to_linear(cs, ip);

	switch(ip_addr[0])
	{
		case 0xcd:
			print("INT \n");
			break; 
	}

	while(1);
	kernel_panic("GENERAL_PROTECTION_FAULT");
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
