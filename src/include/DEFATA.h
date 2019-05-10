#ifndef DEFATA_H
#define DEFATA_H

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATAPI_PACKET 0xA0

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

#define	SATA_SIG_ATA	0x00000101	
#define	SATA_SIG_ATAPI	0xEB140101


#define SYS_PATA 1
#define SYS_SATA 2
#define SYS_PATAPI 3
#define SYS_SATAPI 4

#endif