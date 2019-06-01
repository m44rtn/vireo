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
void (*func_ptr) (uint32_t);
uint32_t KERNEL_FLAGS = 0;

void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs)
{
	//announce ourselves
	clearscr();	
	
	kernel_version(); //release, major, minor, build
	//setup the segments
	segments.cs = cs;
	segments.ss = ss;

	//Prepare the TSS and GDT
	Prep_TSS();
	GDT();

	systeminfo.mouseX = systeminfo.mouseY = 0;
	//ps2_mouse_init(); //is actually in keyboard.c, which may be renamed to ps2 in the future

	//Setup interrupts                             
	setints();	
	

	//Setup Memory info
	memory_init(mbh);
	pci_init();

	
	//Setup all drive management stuff
	print("\nDetecting master type...\n");
	systeminfo.master = ATA_init(0); //search for ATA devices
	print("\nDetecting slave type...\n");
	systeminfo.slave = ATA_init(1); //search for ATA devices
	trace("Master is type: %i\n", systeminfo.master);
	trace("Slave is type: %i\n", systeminfo.slave);

	if(systeminfo.master == 0 && systeminfo.slave == 0) kernel_panic("DRIVE_NOT_FOUND");
		
	//drive testing stuff
	
	get_drive_info();
	FATinit(vfs_info.HD0);
	uint8_t drive = vfs_info.HD0;

	//print("Press enter to continue...\n");
	//hang_for_key(KEYB_ENTER);
	//clearscr();
	/*vesa_init(1024, 768, 32); 

	for(uint32_t y = 0; y < 768; y++)
	{
		for(uint32_t x = 0; x < 1024; x++)
		{
			vesa_put_pixel(x, y, 0x23272a);
		}
	}*/
	
	ps2_keyb_init();
	systeminfo.KEYB_OnScreenEnabled = true;
	
	//while(1);
	//task_push(TASK_HIGH, (uint32_t) task1, TASK_FLAG_KERNEL);
	//task_push(TASK_HIGH, (uint32_t) task2, TASK_FLAG_KERNEL);

	
	//asm_stuff();
	
	asm __volatile__("mov $30, %eax\n" "int $3");
	
	while(1);
	tREGISTERS *registers;
	kmemset(registers, 0, sizeof(tREGISTERS));
	//systeminfo.FLAGS = INFO_FLAG_MULTITASKING_ENABLED;
	jmp_user_mode(kernel_thing, registers);
	//clearscr();

	while(1);
}

void kernel_thing()
{
	print("\nRed Alert!\n");
	//kernel_panic_dump("LCARS_RED_ALERT");
	while(1);
}

void kernel_version()
{
	trace(" Vireo kernel v%s.", (int) intstr(RELEASE));
	trace("%s.", (int) intstr(MAJOR));
	trace("%s.", (int) intstr(MINOR));
	trace("%s ", (int) intstr(BUILD));
	print("x86\n\n");
}


