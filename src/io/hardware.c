#include "hardware.h"


//PIC
#define PIC1 0x20                                    //PIC1 command port
#define PIC2 0xA0                                    //PIC2 command port
#define PIC1DATA (PIC1 + 1)                          //PIC1 data port
#define PIC2DATA (PIC2 + 1)                          //PIC2 data port

//ICW1
#define ICW1 0x11
#define ICW1_ICW4 0x01                              //ICW4 (not) needed
#define ICW1_SINGLE 0x02                            //single (cascade) mode
#define ICW1_INTERVAL4 0x04                         //call adress interval 4 (8)
#define ICW1_LEVEL 0x08                             //Level triggered (edge)
#define ICW1_INIT 0x10                              //initialization

//ICW4
#define ICW4 0x01
#define ICW4_8086 0x01                             //8086 and/or 8088 mode
#define ICW4_AUTO 0x02                             //AUTO end of interrupt (EOI)
#define ICW4_BUF_SLAVE 0x08                        //Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C                       //buffered mode/master
#define ICW4_SFNM 0x10                             //special fully nested (not)



//CMOS
int CMOS_NMI_disable = 0x80;


void setints(){				//Initialize the interrupts	

	isrinst();		//setup the ISR
	initpics(0x20, 0x28); //setup the PICs
	PIC_remap(PIC1, PIC2); // remap 'em too
	IRQclrallmsks();
	PIT_initasm(); //That Wasn't much of a rescue!
	asm("sti");
}

void IRQclrmsk(uint8_t line){
	uint16_t port;
	uint8_t value;
		
	if(line < 8){
		port = PIC1DATA;
	}else{
		port = PIC2DATA;
		line -= 8;
	}
	
	value = inb(port) & ~(1 << line);
	outb(port, value);
	
}
void IRQclrallmsks(){
	outb(0x20, 0x00);
	outb(0xA0, 0x00);
}

void IRQmskall(){
	uint8_t in1 = inb(PIC1DATA) & 0xff;
	uint8_t in2 = inb(PIC2DATA) & 0xff;
	outb(PIC1DATA, in1);
	outb(PIC2DATA, in2);
}

void IRQdisableall(){
	outb(PIC1DATA, 0xFF);
	outb(PIC1DATA, 0xFF);
}

void IRQsetmsk(uint8_t line){
	uint16_t port;
	uint8_t value;
		
	if(line < 8){
		port = PIC1DATA;
	}else{
		port = PIC2DATA;
		line -= 8;
	}
	
	value = inb(port) | (1 << line);
	outb(port, value);
	
}

void initpics(int pic1, int pic2){
	//send to ICW1
	outb(PIC1, ICW1);
	outb(PIC2, ICW1);
	
	//send to ICW2
	outb(PIC1DATA, 0x20);
	outb(PIC2DATA, 0x28);
	
	//send to ICW3
	outb(PIC1DATA, 4);
	outb(PIC2DATA, 2);
	
	//send to ICW4
	outb(PIC1DATA, 0x05);
	outb(PIC2DATA, ICW4);
	
	//disable the IRQs
	outb(PIC1DATA, 0x00);
	outb(PIC2DATA, 0x00);
	
	outb(PIC1DATA, 0xFC);
	outb(PIC2DATA, 0xFC);
}

void inthndld(unsigned char irq){    //say to the PICs that the int is handled by the OS
	if (irq >= 8) outb(PIC2, 0x20);
	outb(PIC1, 0x20);
}


void PIC_remap(int offset1, int offset2){
	
	unsigned char a1, a2;
	a1 = inb(PIC1DATA);              // save the masks
	a2 = inb(PIC2DATA);
	
	outb(PIC1, ICW1_INIT+ICW1_ICW4); //starts the init in cascade mode
	outb(PIC2, ICW1_INIT+ICW1_ICW4);
	outb(PIC1DATA, PIC1);         //vector offset master
	outb(PIC2DATA, PIC2);         //vector offset slave
	outb(PIC1DATA, 4);               //Master: there is a slave PIC at irq2 (0000 0100)
	outb(PIC2DATA, 2);               //Slave: I tell you the cascade identity (0000 0010)
	
	outb(PIC1DATA, ICW4_8086);
	outb(PIC2DATA, ICW4_8086);
	
	outb(PIC1DATA, a1);             //restore evrything
	outb(PIC2DATA, a2);
	
}

void PIT_init(){ 						//LOL

	int PIT_freq  = 1193180 / 256;                    //Frequency of the PIT
	outb(0x43, 0x36);//0b00110110);
	outb(0x40, PIT_freq & 0xffff);
	outb(0x40, PIT_freq >> 8);
}

void inita20(void){
	
	uint8_t a;
	
//	asm("cli"); //disable interrupts
	
	keybwait();
	keybsndcom(0xAD);
	
	keybwait();
	keybsndcom(0xD0);
	
	keybwait();
	a = inb(0x60);
	
	keybwait();
	keybsndcom(0xD1);
	
	keybwait();
	outb(0x60, a | 2);
	
	keybwait();
	keybsndcom(0xAE);
	
//	asm("sti"); //enable interrupts
//	isrinst();
//	PICinit();
	
	
}

void DetectSpeed(){
	int i = systeminfo.PITcount;
	int j = 0;
	int y = 0;
	int x = 0;
	i--;
	for(i; i != 100; i++){ 
		j++; j++; j++;
		x = systeminfo.PITcount;
		}
	
	systeminfo.nItterations = j;
}

void keybsndcom(uint8_t com){
	outb(0x64, com);
}

void keybwait(){
	while(inb(0x64) != 2){}
}







