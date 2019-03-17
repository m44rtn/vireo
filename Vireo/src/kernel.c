#include "kernel.h"



void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs)
{
	
	//announce ourselves
	clearscr();	
	print("Starting...\n");

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
	AHCI_init(); //search for AHCI devices (although unsupported)

	print("\nDetecting master type...\n");
	systeminfo.master = ATA_init(0); //search for ATA devices
	print("\nDetecting slave type...\n");
	systeminfo.slave = ATA_init(1); //search for ATA devices
	systeminfo.slave = SYS_PATAPI;


	print("\n");
	trace("Master is type: %i\n", systeminfo.master);
	trace("Slave is type: %i\n", systeminfo.slave);

	if(systeminfo.master == 0 && systeminfo.slave == 0) kernel_panic("DRIVE_NOT_FOUND");
		
	
	//drive testing stuff
	uint8_t drive;

	if(systeminfo.master == SYS_PATA) drive = 0;
	else if(systeminfo.slave == SYS_PATA) drive = 1;

	FATinit(drive);
	//uint32_t clust = FAT_Traverse("HD0/FDOS/BIN");
	
	//uint32_t leng = 0;
	//FAT32_DIR *dir = ReadDir(drive, clust, &leng);
	uint32_t len = 0;
	FAT32_VFS *vfs = FATFindFreeClusterChain(0, 10 * 4096, &len);
	trace("length of the 'found clusters' chain: %i\n", len);
	
	for(int i = 0; i < len; i++)
	{
		trace("found free cluster: %i\n", vfs[i].cluster);
	}

	//char *file = malloc(512);
	//char *str = "Hello, this is a file made and stored by BirdOS.";
	//strcopy(file, str);

	//FAT32_WRITE_FILE(drive, file, 512, "BIRDOS  ", "HD0/");

	uint32_t *thing = FindDriver("VESA    SYS ");

	trace("size of bool: %i\n", sizeof(bool));

	print("Press enter to continue...\n");
	hang_for_key(KEYB_ENTER);
	clearscr();

	//release, major, minor, build
	trace(" Vireo kernel %s x86\n\n", (int) "v0.5.5.140");

	
	print("\nRed Alert!\n");
	print("Hold it right there, pardner!\n");
	while(1);
}