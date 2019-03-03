#ifndef AHCI_H
#define AHCI_H

#include "../../include/types.h"
#include "../../io/sys.h"
#include "../../include/info.h"
#include "../../io/PCI.h"
#include "../../include/DEFATA.h"

void GetSATADevicesPCI();

typedef enum{
	FIS_TYPE_REG_H2D = 0X27,		//register host to device
	FIS_TYPE_REG_D2H = 0X34,	//register device to host
	FIS_TYPE_DMA_ACT = 0X39,	//dma activate device to host
	FIS_TYPE_DMA_SETUP = 0X41, //dma activate bidirectional
	FIS_TYPE_DATA = 0X46,	//data FIS bidirectional
	FIS_TYPE_BIST = 0X58,	//BIST activate FIS bidirectional
	FIS_TYPE_PIO_SETUP = 0X5F,	//PIO device to host
	FIS_TYPE_DEV_BITS = 0XA1,	//set device bits, device to host
} FIS_TYPE;

typedef struct tagFIS_REG_H2D{
	//dword 0
	uint8_t fis_type;
	uint8_t pmport:4;
	uint8_t rsv0: 3;
	uint8_t c: 1; //1 = command, 0 = control
	uint8_t command; //command register
	uint8_t feature_reg1;
	
	//dword 1
	uint8_t lba0; //low
	uint8_t lba1; //middle
	uint8_t lba2; //high
	uint8_t device;
	
	//dword 2
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feature_reg2;
	
	//dword3
	uint8_t countl; //low
	uint8_t counth; //count high
	uint8_t icc; //isochronous command completion
	uint8_t control; //nou, da's wel  duidelijk hé?
	
	//dword4
	uint8_t resv[4]; //reserved
} FIS_REG_H2D;

typedef struct tagFIS_REG_D2H{
	uint8_t fistype;
	uint8_t pmport:4;
	uint8_t resv: 2;
	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t resv2;
	
	uint8_t countlo;
	uint8_t counthi;
	uint8_t resv3[2];
	
	uint8_t resv4;
} FIS_REG_D2H;

typedef struct tagFIS_DATA{
	uint8_t fistype;
	uint8_t pmport:4;
	uint8_t resv0: 4;
	uint8_t resv1;
	
	uint32_t data;
} FIS_DATA;

typedef struct tagFIS_PIO_SETUP{
	uint8_t fistype;
	uint8_t pmport:4;
	uint8_t resv0:1;
	uint8_t d:1; //data  transfer direction (1 = d2h)
	uint8_t i:1; //interrupt
	uint8_t resv1;
	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t resv2;
	
	uint8_t countlo;
	uint8_t counthi;
	uint8_t resv3;
	uint8_t e_status; // new value of status reg
	
	uint16_t tc;
	uint8_t resv4[2];
} FIS_PIO_SETUP;

typedef struct tagFIS_DMA_SETUP{
	uint8_t fistype;
	uint8_t pmport:4;
	uint8_t resv0:1;
	uint8_t d:1; //data  transfer direction (1 = d2h)
	uint8_t i:1; //interrupt
	uint8_t a: 1; //autoactivate --> specifies DMA activate FIS nodig #EN-NL
	uint8_t resv[2];
	
	uint64_t DMAbufferID; //buffer ID (first uint64 used ever!!), used to indentify DMA buff in memory (host specific?)  |  DWORD1&2
	
	uint32_t resv2;
	
	uint32_t DMAbuffOffset; //1st 2 bits always 0
	
	uint32_t transfercount; //1st bit always 0
	
	uint32_t resv3;
}FIS_DMA_SETUP;

typedef volatile struct tagHBA_PORT{ //uint32 = 0x04 big
 	uint32_t clb; //command list base address 1000 byte aligned
	uint32_t clbu; //command list base address upper 32 bits
	uint32_t fb; //FIS base adress 256-byte aligned
	uint32_t fbu; //FIS base adress upper 32 bits
	uint32_t is; //interrupt status
	uint32_t ie; //interrupt enable
	uint32_t cmd; //command and status
	uint32_t rsv; //reserved
	uint32_t tfd; //task file data
	uint32_t sig; //signature
	uint32_t ssts; //SATA status
	uint32_t sctl; //SATA control
	uint32_t serr; //SATA error
	uint32_t sact; //SATA active
	uint32_t ci; //command issue
	uint32_t sntf; //SATA notification
	uint32_t fbs; //FIS based switch control
	uint32_t rsv1[11]; //reserved
	uint32_t vendor[4]; //vendor specific
}HBA_PORT;

typedef volatile struct tagHBA_MEM{
	//generic host control 0x00 - 0x2B
	uint32_t cap; // host capabillity 0x00
	uint32_t ghc; //global host control 0x04
	uint32_t is; //interrupt status 0x08
	uint32_t pi; //port implemented 0x0c
	uint32_t vs; //version 0x10
	uint32_t ccc_ctl; // command completion coalescing control 0x14
	uint32_t ccc_pts; //ccc ports 0x18
	uint32_t em_loc; //enclosure management location 0x1c
	uint32_t em_ctl; //enclosure management control 0x20
	uint32_t cap2; //host capabilities extended 0x24
	uint32_t bohc; //bios/os handoff control and status

	//0x0a-0x2c reserved
	uint8_t resv[0xA0-0x2C];

	//0x0a-0xff vendor specific
	uint8_t vendor[0x100-0xA0];

	//0x100-0x10ff port control regs
	HBA_PORT ports[1];
}HBA_MEM;

typedef struct tagHBA_CMD_HEADER{
	uint8_t cfl:5;
	uint8_t a:1; //atapi
	uint8_t w:1; //write, h2d = 1 and d2h = 0
	uint8_t p:1; //prefetchable

	uint8_t reset:1;
	uint8_t BIST:1;
	uint8_t c:1; //clear busy upon R_OK
	uint8_t resv1:1;
	uint8_t pmp; //port multiplier port
	uint16_t prdtlen; //Physical region descriptor table length in entries

	volatile uint32_t prdbc; // physical region descriptor table byte count transferred

	uint32_t ctba; //command table descriptor base address
	uint32_t ctbau; // " upper 32-bits

	uint32_t resv2[4];

}HBA_CMD_HEADER;

typedef struct tagHBA_PRDT_ENTRY{
	uint32_t dba; //database address
	uint32_t dbau; //upper 32-bits
	uint32_t resv;

	uint32_t dbc:22; //byte count 4m max
	uint32_t resv1:9;
	uint32_t i:1; //interrupt on completion 
}HBA_PRDT_ENTRY;

typedef struct tagHBA_CMD_TBL{

	uint8_t cfis[64];
	uint8_t acmd[16];
	uint8_t resv[48];
	HBA_PRDT_ENTRY prdt[1];
} HBA_CMD_TBL;



typedef struct{
    uint32_t ID;
    uint8_t portnum;
} AHCI_Devices;
AHCI_Devices *devices[32];


void AHCI_init();
uint32_t getBAR5(bool ignoremsg);

AHCI_Devices *search_drives(HBA_MEM *abar);

void AHCI_setup_port();

static int AHCI_check_type(HBA_PORT *port);
void start_cmd(HBA_PORT *port);
void stop_cmd(HBA_PORT *port);
void port_rebase(HBA_PORT *port, int portno);

void Read_SATA(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t* buf, uint32_t count);
int find_cmdslot(HBA_PORT *port);
HBA_MEM *abar;

#endif