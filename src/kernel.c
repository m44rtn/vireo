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
#include "drivers/v86.h"
#include "io/multitasking.h"
#include "include/DEFATA.h"
#include "include/GRUB/multiboot.h" //mutliboot stuff --> grub
#include "include/version.h"
#include "drivers/screen/VESA.h"

void kernel_version();

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

	//Setup interrupts                             
	setints();	
	//systeminfo.FLAGS = 0;

	//Setup Memory info
	memory_init(mbh);
	
	//Setup all drive management stuff
	print("\nDetecting master type...\n");
	systeminfo.master = ATA_init(0); //search for ATA devices
	print("\nDetecting slave type...\n");
	systeminfo.slave = ATA_init(1); //search for ATA devices
	systeminfo.slave = SYS_PATAPI;
	trace("Master is type: %i\n", systeminfo.master);
	trace("Slave is type: %i\n", systeminfo.slave);

	if(systeminfo.master == 0 && systeminfo.slave == 0) kernel_panic("DRIVE_NOT_FOUND");
		
	
	//drive testing stuff
	
	get_drive_info();
	//trace("drive = HD%i\n", vfs_info.HD0);
	FATinit(vfs_info.HD0);
	uint8_t drive = vfs_info.HD0; //vfs_info.HD0;
	
	//apparantly this is necesarry
	uint32_t len = 0;

	vesa_findmode(320, 480, 2);
	//uint32_t *thing = FindDriver("VESA    SYS"); //lot's of errors
	
	//clearscr();
	
	

	/* setting up the test tasks */
	/*uint32_t *task1 = malloc(4096);
    File *file = FindFile("TASK1   SYS", vfs_info.SYSFOLDER_CLUST, drive);
    uint32_t lba = FAT_cluster_LBA(file->FileLoc);
    PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) task1);



	uint32_t *task2 = malloc(4096);
    File *file2 = FindFile("TASK2   SYS", vfs_info.SYSFOLDER_CLUST, drive);
    uint32_t lba2 = FAT_cluster_LBA(file2->FileLoc);
    PIO_READ_ATA(0, lba2, ((file2->size / 512) + 1), (uint16_t *) task2);
	
	/*uint32_t *task3 = malloc(512);
	uint32_t DIRLOC = FAT_Traverse("HD0/BIRDOS/");
    File *file = FindFile("TASK3   SYS", DIRLOC, drive);
    uint32_t lba = FAT_cluster_LBA(file->FileLoc);
    PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) task2);*/
	
	/*trace("task 1 eip=%i\n", (uint32_t) task1);
	task_push(TASK_HIGH, (uint32_t) task1, TASK_FLAG_KERNEL);
	task_push(TASK_HIGH, (uint32_t) task2, TASK_FLAG_KERNEL);
	//clearscr();
	sleep(1);
	systeminfo.FLAGS = KERNEL_FLAGS = INFO_FLAG_MULTITASKING_ENABLED;*/
	
	//task_push(TASK_HIGH, (uint32_t) task3, NULL);

	while(1);


	print("Press enter to continue...\n");
	hang_for_key(KEYB_ENTER);
	clearscr();
	
	print("\nRed Alert!\n");
	print("Hold it right there, pardner!\n");
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


