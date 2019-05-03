#include "kernel.h"

uint32_t KERNEL_FLAGS = 0;

void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs)
{
	//announce ourselves
	clearscr();	
	trace(" Vireo kernel %s x86\n\n", (int) "v0.5.5.260"); //release, major, minor, build
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

	//uint32_t *thing = FindDriver("VESA    SYS"); //lot's of errors
	
	//clearscr();
	

	/* setting up the test tasks */
	uint32_t *task1 = malloc(4096);
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
	
	trace("task 1 eip=%i\n", task1);
	task_push(TASK_HIGH, (uint32_t) task1, TASK_FLAG_KERNEL);
	task_push(TASK_HIGH, (uint32_t) task2, TASK_FLAG_KERNEL);
	//clearscr();
	sleep(1);
	systeminfo.FLAGS = KERNEL_FLAGS = INFO_FLAG_MULTITASKING_ENABLED;
	
	//task_push(TASK_HIGH, (uint32_t) task3, NULL);

	while(1);


	print("Press enter to continue...\n");
	hang_for_key(KEYB_ENTER);
	clearscr();
	
	print("\nRed Alert!\n");
	print("Hold it right there, pardner!\n");
	while(1);
}


