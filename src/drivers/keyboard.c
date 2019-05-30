#include "keyboard.h"

#define ACK (int) 0xFA


char* key_bfr;
char *key_bfr_codes; //for all the pressed key codes
uint32_t code_bfr_loc = 0;

bool shift = false;

int scancodeset = 1;

char *keyboard_chars = "  1234567890-=\b\tqwertyuiop[]\n asdfghjkl;'` \\zxcvbnm,./    ";

void keybin(char key){
	systeminfo.LastKey = key;
	if(systeminfo.KEYB_OnScreenEnabled) keyboard_putonscr(key);
	key_bfr_codes[code_bfr_loc] = key;
	if(code_bfr_loc < 512) code_bfr_loc++;	
	else code_bfr_loc = 0;
}

void keyboard_putonscr(char key)
{
	bool show_char = false; // this helps us ignore SHIFT 'n such (otherwise they show up as a space)
	if(key < KEYB_ONE || key > KEYB_SPACE) return;
	
	char c = keyboard_chars[key];

	//bug: thing only works once (shift never becomes false again) -- but now it doesn't work at all...
	if(key == KEYB_LEFT_SHIFT_PRESSED || key == KEYB_RIGHT_SHIFT_PRESSED) shift = true;
	else if(key == KEYB_LEFT_SHIFT_RELEASED || key == KEYB_RIGHT_SHIFT_RELEASED) shift = false;
	else show_char = true;
	
	if(c == '\b' && !(systeminfo.key_bfr_loc > 0)) return;
	else if(c == '\b') 
	{
		systeminfo.key_bfr_loc--;
		key_bfr[systeminfo.key_bfr_loc] = '\0';
		putchar(c);	
		return;
	}

	if(shift)
	{
		c = util_c_transform_uc(c, UTIL_UPPERCASE);
		shift = false;
	}

	if(show_char && (systeminfo.key_bfr_loc < 512))
	{
		key_bfr[systeminfo.key_bfr_loc] = c;
		systeminfo.key_bfr_loc++;
		putchar(c);

		if(c == '\n') systeminfo.key_bfr_loc = 0;
	}
}

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
	/*Set Keyboard scancode set
	uint8_t status; 
	ps2_wait_write();
	outb(0x60, 0xff);
	ps2_wait_read();
	//if(inb(0x60) != 0xFA) return;
	while(inb(0x60) & 0xAA != 0xAA);*/
	systeminfo.key_bfr_loc = 0;
	key_bfr = (char*) malloc(512);
	key_bfr_codes = (char*) malloc(512);
	
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
