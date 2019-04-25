#include "HDD.h"


void get_drive_info()
{
	uint8_t root_drive;
	uint8_t master, slave;
	uint32_t result;
	
	if(systeminfo.slave == SYS_PATA)
	{ 
		FATinit(1);
		
		vfs_info.HD0 = 1;
		extern uint32_t mem_table_entry;

		uint32_t LOC = FAT_Traverse("HD0/ROOT/");

    	File *file = (File *) fat_find_dir(1, "BIRDOS", LOC);
    	uint32_t lba = FAT_cluster_LBA(file->FileLoc);
		uint16_t *buf = malloc(file->size);

		vfs_info.SYSFOLDER_LBA = lba;
		vfs_info.SYSFOLDER_CLUST = file->FileLoc;
		
    	PIO_READ_ATA(1, lba, ((file->size / 512) + 1), (uint16_t *) buf);

		if (result < 0x0FFFFFF8) root_drive = 1;
	}
	
	if(systeminfo.master == SYS_PATA)
	{ 
		FATinit(0);
	
		vfs_info.HD0 = 0;
		extern uint32_t mem_table_entry;

		uint32_t LOC = FAT_Traverse("HD0/ROOT/");

    	File *file = (File *) fat_find_dir(0, "BIRDOS", LOC);
    	uint32_t lba = FAT_cluster_LBA(file->FileLoc);
		uint16_t *buf = malloc(file->size);

		vfs_info.SYSFOLDER_LBA = lba;
		vfs_info.SYSFOLDER_CLUST = file->FileLoc;

    	PIO_READ_ATA(0, lba, ((file->size / 512) + 1), (uint16_t *) buf);
		if (result < 0x0FFFFFF8) root_drive = 0;
		
	}
	
	vfs_info.HD0 = root_drive;
}

uint32_t GetFirstSectLBA(uint8_t drive, uint8_t DriveType)
{

	//REDO THIS THING

	//Returns first sector of the 'partition'
	//However it searches only the first hundred 

	uint16_t *buf = malloc(512);

	switch(DriveType)
	{

		case SYS_PATA:
			PIO_READ_ATA(drive, 0, 1, buf);
			if(buf[255] == 0xAA55) return 0;
			
			PIO_READ_ATA(drive, 63, 1, buf);
			if(buf[255] == 0xAA55) return 63;
		break;

	}

	return -1;

}