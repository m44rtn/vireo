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
	systeminfo.LastKey = 0;
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

void mouse_write(uint8_t byte)
{
	mouse_wait(1);
	outb(0x64, 0xD4);

	mouse_wait(1);
	outb(0x60, byte);
}

uint8_t mouse_read()
{
	mouse_wait(0);
	return inb(0x60);
}

void mouse_wait(uint8_t type)
{
	uint32_t timeout = 100000;

	if(type == 0)
	{
		while(timeout--)
			if( (inb(0x64) & 1) == 1) return;
	}
	else
	{
		while(timeout--)
			if( (inb(0x64) & 2) == 0) return;
	}

}

void ps2_mouse_init()
{	
	//enable the thing
	uint8_t status;

	mouse_wait(1);
	outb(0x64, 0xA8);
	print("A8\n");

	mouse_wait(1);
	outb(0x64, 0x20);
	print("20\n");

	mouse_wait(0);
	status = inb(0x60) | 2;
	print("got status\n");

	mouse_wait(1);
	outb(0x64, 0x60);
	mouse_wait(1);
	outb(0x60, status);

	print("60 and status\n");

	mouse_write(0xff);
	print("reset\n");

	mouse_write(0xF6);
	mouse_read();
	print("F6\n");

	mouse_write(0xF4);
	mouse_read();
	print("F4\n");

	//IRQclrmsk(12);	
}


void ps2_wait_write()
{
	while(inb(0x64) & 0x02 != 0x00);
}

void ps2_wait_read()
{
	while(inb(0x64) & 0x01 != 0x01);
}
