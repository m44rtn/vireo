#include "keyboard.h"

#define ACK (int) 0xFA


bool enter = 0;
int bEnterPause = 0;
int boolsk3 = 0;
char buff[256];
char* kbrdbfr = (char*) &buff; 
//systeminfo.keybrdbfrloc = 0;
int noreturn = 0;
int shift = 0;
int scancodeset = 1;



void keybin(char key){
	systeminfo.LastKey = key;
	if(systeminfo.KEYB_OnScreenEnabled) putonscr(key);
}

void putonscr(char chr){}

bool IS_pressed(char key){
	if(key == systeminfo.LastKey) return true;
	return false;
}

void hang_for_key(char key){
	while(!IS_pressed(key));
}

void ps2_keyb_init()
{
	/*Set Keyboard scancode set*/
	uint8_t status; 
	ps2_wait_write();
	outb(0x60, 0xff);
	ps2_wait_read();
	//if(inb(0x60) != 0xFA) return;
	while(inb(0x60) & 0xAA != 0xAA);
	
}

void ps2_mouse_init()
{	
	//enable the thing
	ps2_wait_write();

	outb(0x64, 0xad);
	
	ps2_wait_write();
	outb(0x64, 0xa8);
	
	ps2_wait_write();
	outb(0x64, 0xd4);

	ps2_wait_write();
	outb(0x60, 0xff);

	ps2_wait_read();
	uint8_t status = inb(0x60);
	while((inb(0x60) & 0xAA) != 0xAA) ps2_wait_read();
	
	ps2_wait_read();
	status = inb(0x60); //read mouseID?

	ps2_wait_write();
	outb(0x64, 0x20);

	ps2_wait_read();
	status = inb(0x60);
	trace("status=%i\n", status);
	status |= 0x02;
	//status &= 0xffdf;
	
	ps2_wait_write();
	outb(0x64, 0x60);
	ps2_wait_write();
	outb(0x60, status);
	
	ps2_wait_write();
	outb(0x60, 0xF4);
	
	ps2_wait_write();
	outb(0x64, 0xAE);
	
	print("MOUSE INIT COMPLETE\n");
}


void ps2_wait_write()
{
	while(inb(0x64) & 0x02 != 0x00);
}

void ps2_wait_read()
{
	while(inb(0x64) & 0x01 != 0x01);
}
