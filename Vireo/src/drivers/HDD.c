#include "HDD.h"

uint32_t GetFirstSectLBA(uint8_t drive, uint8_t DriveType)
{
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