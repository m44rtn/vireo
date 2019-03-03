#ifndef ATA_H
#define ATA_H

#include "../../include/types.h"
#include "../../io/PCI.h"
#include "../../io/isr.h"
#include "../../include/DEFATA.h"
#include "../../include/info.h"
#include "../../io/sys.h"
#include "../cpu.h"
#include "../../io/hardware.h"


#define ATA_PRIMARY_DRIVE_DATAP 0x1F0
#define ATA_PRIMARY_DRIVE_FEATURES 0x1F1
#define ATA_PRIMARY_DRIVE_SCTRCNTP 0x1F2
#define ATA_PRIMARY_DRIVE_LBALOWP 0x1F3
#define ATA_PRIMARY_DRIVE_LBAMIDP 0x1F4
#define ATA_PRIMARY_DRIVE_LBAHIP 0x1F5
#define ATA_PRIMARY_DRIVE_SELECTP 0x1F6
#define ATA_PRIMARY_DRIVE_COMSTAT 0x1F7 //command and status port

uint8_t ATA_init(uint8_t drive);
void ATA_swreset();
void ATAwait();
bool ATAPI_CHECK(uint8_t drive);
void nIEN_rs();

extern void isr2e();
extern void isr2F();


void PIO_READ_ATA(uint8_t drive, uint32_t start, uint8_t sctrWRITE, uint16_t *buf);
void PIO_WRITE_ATA(uint8_t drive, uint32_t start, uint8_t sctrWRITE, uint16_t *buf);
void PIO_READ_ATAPI(uint8_t drive, uint32_t start, uint32_t sctrWRITE, uint16_t *buf, uint16_t bufSize);


void DMA_ATA_SETUP();
void DMA_ATA_READ(uint8_t drive, uint32_t *buffer, uint32_t start, uint8_t nSectors);

void ATA_STANDARD();



#endif