#include "error.h"

void kernel_panic(char* error){
	clearscr();
	
	paintscr(0x70);
	print("Your PC is being shutdown to prevent any further damage to your computer.\n\n");
	print("Error:\n\t");
	print(error);
	print("\n\n\n");
	
	print("Please proceed as follows:\n");
	print("\t- If this is the first time you see this error screen, please restart\n\tyour computer.\n");
	print("\t- If this problem happens again please uninstall any new hardware and/or\n\tsoftware.\n");
	print("\t- If the problem still exists send the error named above\n\tand a description of what you were doing before");
	print(" this happened\n\tto FeatherCode.");
	
	print("\n\n\nEmail: birdos.2015@gmail.com\n");
	print("For more contact information see the website: http://feathercode.github.io");
	print("\n\nBy sending this info we can solve the problem in future builds.\n");
	disablecursor();
	while(1);
}

void InitFail(char* fail){
			//add screen color, black and yellow
	setcolor(0x0E);
	print(fail);
	print(" initialization failed, some functions may not work properly\n");
	sleep(10);
	setcolor(0x07);
}
void console_Warning(char* warning){
	setcolor(0x0E);
	print(warning);
	print("\n");
	setcolor(0x07);
}

void error(int err){
	setcolor(0x04);
	print("[Error]");
	setcolor(0x07);
	switch(err){
		case 0:
			print(" Unknown error (code 0)\n");
			break;
		case 6:
			print(" Drive number not in range (code 6)\n");
			break;
		case 54:
			print(" Out of memory (code 54)\n");
			break;
		case 55:
			print(" Memory not detected (code 55)\n"); //first error code implemented, based on POST code 55 (memory not detected)
			sleep(300);
			kernel_panic("MEMORY_NOT_DETECTED");
			break;
		case 56:
			print(" Illegal operation (code 56)\n");
			asm("hlt");
			break;
		case 57:
			print(" Failed allocating memory (code 57)\n");
			break;
		case 58:
			print(" Unable to find the specified PCI device (code 58)\n");
			break;
		case 404:
			print(" File not Found (code 404)\n");
			break;
	}
}
