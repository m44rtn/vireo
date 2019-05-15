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

void InitKbrd(){
	/*Set Keyboard scancode set*/
	uint8_t Keyboard_Reply;
	Keyboard_Reply = inb(0x64);
	//outb(0x64, 0x14);
	outb(0x64, 0xF0 | 0);
	Keyboard_Reply = inb(0x64);
	scancodeset = inb(0x64);
	
	if(Keyboard_Reply == ACK){
		print("Acknowledged\n");
		sleep(200);
		scancodeset = inb(0x64);
		
	}else if(Keyboard_Reply == 0xFE){
		sleep(200);
	}else{
	
	}
	
}

void ps2_mouse_init()
{
	//should wait with sending before sending, but doesn't yet
	
	outb(0x64, 0xa8);
	while((inb(0x60) & 0xFA) != 0xFA);

	outb(0x60, 0xff);
	while((inb(0x60) & 0xFA) != 0xFA);
	//outb(0x60, 0xf6);
	//while((inb(0x60) & 0xFA) != 0xFA);

	//enable the thing
	outb(0x64, 0x20);

	uint8_t status = inb(0x64);
	status |= 0x02;
	status &= 0xffdf;

	outb(0x64, 0x60);
	outb(0x60, status);

	
	
	print("MOUSE INIT COMPLETE\n");
}


