/*
Copyright (c) 2019 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 */ 

#include "include/types.h"
#include "drivers/screen/screen.h"
#include "io/gdt.h"
#include "drivers/cpu.h"
#include "io/isr.h"
#include "io/hardware.h"
#include "io/util.h"
#include "drivers/keyboard.h"
#include "io/memory.h"
#include "include/info.h"
#include "include/keych.h"
#include "include/error.h"
#include "io/PCI.h"
#include "drivers/ATA/SATA.h"
#include "drivers/ATA/ATA.h"
#include "drivers/FS/FAT.h"
#include "drivers/FS/ISO9660.h"
#include "drivers/HDD.h"
#include "drivers/DriverHandler.h"
#include "io/v86.h"
#include "io/multitasking.h"
#include "include/DEFATA.h"
#include "include/GRUB/multiboot.h" //mutliboot stuff --> grub
#include "include/version.h"
#include "drivers/screen/VESA.h"

void kernel_thing();
void kernel_version();
void main_kernel_shell_startup();
uint32_t KERNEL_FLAGS = 0;

void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs)
{
	//announce ourselves
	clearscr();	
	kernel_version(); // major, minor, build

	//setup the segments info
	segments.cs = cs;
	segments.ss = ss;

	//Prepare the TSS and GDT
	Prep_TSS();
	GDT();

	//systeminfo.mouseX = systeminfo.mouseY = 0;
	//ps2_mouse_init(); //is actually in keyboard.c, which may be renamed to ps2.c in the future

	//Setup interrupts                             
	setints();	
	
	//Setup Memory info
	memory_init(mbh);
	pci_init();

	
	//Setup all drive management stuff
	//print("\nDetecting master type...\n");
	systeminfo.master = ATA_init(0); //search for ATA devices
	//print("\nDetecting slave type...\n");
	systeminfo.slave = ATA_init(1); //search for ATA devices
	//trace("Master is type: %i\n", systeminfo.master);
	//trace("Slave is type: %i\n", systeminfo.slave);

	if(systeminfo.master == 0 && systeminfo.slave == 0) kernel_panic("DRIVE_NOT_FOUND");
		
	//drive testing stuff
	
	get_drive_info();
	FATinit(vfs_info.HD0);
	uint8_t drive = vfs_info.HD0;
	
	ps2_keyb_init();
	
	//char *response = user_ask("Please enter the number of the PATAPI device [0/1]: ");

	main_kernel_shell_startup();	

	while(1);
}

void kernel_version()
{
	print(" Vireo kernel v");
	trace("%s.", (int) intstr(MAJOR));
	trace("%s.", (int) intstr(MINOR));
	trace("%s ", (int) intstr(BUILD));
	print("x86\n\n");
}


void main_kernel_shell_startup()
{
	//this is here until it gets it's own file
	extern char *key_bfr;
	systeminfo.KEYB_OnScreenEnabled = true;
	print("No configuration file found, please provide the path to one: ");

	while(1)
	{
		if(key_bfr[systeminfo.key_bfr_loc - 1] == '\n')
		{ 
			//main_get_config_file(key_bfr);			
		}
	}	
}

void main_get_config_file(char *path)
{
	uint32_t cluster = FAT_Traverse(path);
	//File* config_file = FindFile()
}