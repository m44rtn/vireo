#include "kernel.h"



void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs)
{
	//announce ourselves
	clearscr();	
	trace(" Vireo kernel %s x86\n\n", (int) "v0.5.5.193"); //release, major, minor, build

	//setup the segments
	segments.cs = cs;
	segments.ss = ss;

	//Prepare the TSS and GDT
	Prep_TSS();
	GDT();

	//Setup interrupts                             
	setints();	

	//Setup Memory info
	GRUB_GetMemInfo(mbh);
	
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
	uint8_t drive;

	if(systeminfo.master == SYS_PATA) drive = 0;
	else if(systeminfo.slave == SYS_PATA) drive = 1;

	FATinit(drive);

	//apparantly this is necesarry
	uint32_t len = 0;
	FAT32_VFS *vfs = FATFindFreeClusterChain(0, 10 * 4096, &len);

	//uint32_t *thing = FindDriver("VESA    SYS "); //lot's of errors

	clearscr();

	/* setting up the test tasks */
	uint32_t *task1 = malloc(512);
	uint32_t LOC = FAT_Traverse("HD0/BIRDOS/");
    File *file = FindFile("TASK1   SYS", LOC, drive);
    uint32_t lba = FAT_cluster_LBA(file->FileLoc);
    PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) task1);


	uint32_t *task2 = malloc(512);
	LOC = FAT_Traverse("HD0/BIRDOS/");
    File *file2 = FindFile("TASK2   SYS", LOC, drive);
    lba = FAT_cluster_LBA(file->FileLoc);
    PIO_READ_ATA(0, lba, ((file2->size / 512) + 1), (uint16_t *) task2);
	

	/*uint32_t *task3 = malloc(512);
	uint32_t DIRLOC = FAT_Traverse("HD0/BIRDOS/");
    File *file = FindFile("TASK3   SYS", DIRLOC, drive);
    uint32_t lba = FAT_cluster_LBA(file->FileLoc);
    PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) task2);*/

	task_push(TASK_HIGH, (uint32_t) task1, NULL);
	task_push(TASK_HIGH, (uint32_t) task2, NULL);
	//task_push(TASK_HIGH, (uint32_t) task3, NULL);



	print("Press enter to continue...\n");
	hang_for_key(KEYB_ENTER);
	clearscr();
	
	print("\nRed Alert!\n");
	print("Hold it right there, pardner!\n");
	while(1);
}

