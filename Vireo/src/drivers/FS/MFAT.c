#include "MFAT.h"

//MFAT driver, yes yes

typedef struct{
    uint8_t jmp;
    uint8_t jmploc;
    uint8_t nop;
    char volname[64];
    char OEMname[8];
    uint8_t attrib;
    uint32_t FAT;
    uint32_t FATsize[2];
    uint32_t totalclust[2];
    char boot[405];
    uint16_t MFATsign;
    uint16_t bootsign;
}tMFAT_MBR;

void MFAT_format(uint8_t drive)
{
    //FORMATS the drive to an MFAT one

    tMFAT_MBR *mbr = malloc(512);

    //set up the bootsector
    mbr->jmp = 0xEB;
    mbr->jmploc = 0x68;
    mbr->nop = 0x90;

    //volume name padded by spaces
    char* namevol = "MFAT is the best FileSystem                                    ";
    for(int i = 0; i < 64; i++)
    {
        mbr->volname[i] = namevol[i];
    }
    
    

    char* nameOEM = "VIREODRV";
    for(int i = 0; i < 64; i++)
    {
        mbr->OEMname[i] = nameOEM[i];
    }
    

    //READ/WRITE volume so attrib is being ignored for now
    //AS IS FAT, FATsize, Totalclust

    for(int i; i < 405; i++) mbr->boot[i] = 0x90; //just fill with nops

    mbr->MFATsign = 0x6446;
    mbr->bootsign = 0xAA55;

    PIO_WRITE_ATA(drive, 65, 1, mbr);
}