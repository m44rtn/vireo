#include "ATA.h"


// 'DEFAULTS' 
#define ATA_PRIMARY_DRIVE_DATAP 0x1F0
#define ATA_PRIMARY_DRIVE_FEATURES 0x1F1
#define ATA_PRIMARY_DRIVE_SCTRCNTP 0x1F2
#define ATA_PRIMARY_DRIVE_LBALOWP 0x1F3
#define ATA_PRIMARY_DRIVE_LBAMIDP 0x1F4
#define ATA_PRIMARY_DRIVE_LBAHIP 0x1F5
#define ATA_PRIMARY_DRIVE_SELECTP 0x1F6
#define ATA_PRIMARY_DRIVE_COMSTAT 0x1F7 //command and status port
#define ATA_PRIMARY_DRIVE_CONTROLP 0x3F6


#define ATA_PORT_SELECT 6
#define ATA_PORT_COMSTAT 7
#define ATA_PORT_LBAHI 5
#define ATA_PORT_LBAMI 4
#define ATA_PORT_FEAT 1

#define ATA_COMMAND_IDENTIFY 0xEC

#define ATAPI_COMMAND_READ 0xA8
#define ATAPI_COMMAND_WRITE 0xAA

#define ATA_COMMAND_DMAREAD 0xC8
#define ATA_COMMAND_DMAWRITE 0xCA


uint8_t ATA_init(uint8_t drive) //0 for master and 1 for slave
{
    uint32_t device;
    systeminfo.waitingforATAPI = false;

    //can we find any devices at all
    if(!GetPCIDevice(0x01, 0x01, true, true) && !GetPCIDevice(0x01, 0x05, true, true)) return 0;

    //if we can, give us the device and if both were found only use the ATA controller
    if(GetPCIDevice(0x01, 0x01, true, true)) device = GetPCIDevice(0x01, 0x01, false, true);
    if(GetPCIDevice(0x01, 0x05, true, true)) device = GetPCIDevice(0x01, 0x05, false, true);
    

    //get the bus, device and function of the ATA controller
    uint8_t bus = getBYTE(device, 2);
    uint8_t dev = getBYTE(device, 1);
    uint8_t fun = getBYTE(device, 0);
    

    //Set the interrupt (not useful at all)
    uint8_t interrupt = pciGetInterruptLine(bus, dev, fun);
    if(interrupt == 0 || interrupt == 0x2E)
    { 
        interrupt = 0x2E;
        uint32_t thing = pciConfigRead(bus, dev, fun, 0x0F) | interrupt;
        pciConfigWrite(bus, dev, fun, 0x0F, thing);
        interrupt = pciGetInterruptLine(bus, dev, fun);
    }
    //trace("interrupt line: %i\n", interrupt);
    IDT_add(interrupt, (uint32_t) ATA_STANDARD);   

    //print("Using PIO mode (slow)...\n");

    //Nuke the controller with a software reset in case it's being weird
    ATA_swreset();

    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | drive << 4);

    //wait 400ns
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);

    //it may help to calc this correctly
    unsigned lo =  inb(ATA_PRIMARY_DRIVE_LBAMIDP);
    unsigned hi = inb(ATA_PRIMARY_DRIVE_LBAHIP);

    //trace("type_lo is: %i\n", lo);
    //trace("type_hi is: %i\n", hi);

    uint16_t type = (hi >> 4) | lo;
    
    //Check the type
    if(hi == 0xEB && lo == 0x14) return SYS_PATAPI;
    if(hi == 0x96 && lo == 0x69) return SYS_SATAPI;
    if(hi == 0 && lo == 0) return SYS_PATA;
    if(hi == 0xC3 && lo == 0x3C) return SYS_SATA;
    return 0;

}

void ATAPI_init(uint8_t drive)
{
    
}

void ATAwait()
{

    while((inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x80 == 0x80));

}

bool ATAPI_CHECK(uint8_t drive)
{
    //a bit weird but okay
    if(drive == 0) outb(ATA_PRIMARY_DRIVE_SELECTP, 0xA0);
    else if(drive == 1) outb(ATA_PRIMARY_DRIVE_SELECTP, 0xB0);

    //wait 400 ns
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);

    outb(ATA_PRIMARY_DRIVE_LBALOWP, 0); 
    outb(ATA_PRIMARY_DRIVE_LBAMIDP, 0);
    outb(ATA_PRIMARY_DRIVE_LBAHIP, 0);
    outb(ATA_PRIMARY_DRIVE_COMSTAT, ATA_COMMAND_IDENTIFY);

    ATAwait();
    int i = inb(ATA_PRIMARY_DRIVE_LBAMIDP);
    int j = inb(ATA_PRIMARY_DRIVE_LBAHIP);
    if(i > 0 && j > 0) return false;
    
    return true;
}

void ATA_swreset(){
    uint8_t value = inb(ATA_PRIMARY_DRIVE_CONTROLP);
    outb(ATA_PRIMARY_DRIVE_CONTROLP, (value | 0x04));
    
    //just wait 400ns again
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);

    outb(ATA_PRIMARY_DRIVE_CONTROLP, 0);

    //why care if you don't have to?
}

bool DRIVE_ERROR_DETECT(uint16_t *buf, uint32_t BufSize_WORDS)
{
    //Returns TRUE if an error is detected
    int j = 0;
    for(int i = 0; i < BufSize_WORDS; i++){
        if(buf[i] == 0xFFFF) j++;
    }
    if(j >= BufSize_WORDS) return true;

    return false;
}

void nIEN_rs(){
    uint8_t control = inb(ATA_PRIMARY_DRIVE_CONTROLP);
    //trace("\ncontrol reg: %i\n", control);
    if(control & 0x02)  //set to receive no interrupts
    {
        outb(ATA_PRIMARY_DRIVE_CONTROLP, (control & 0));
        //print("reset");
    }
}


/* ===================================================================
 *                          PIO MODE
 * ===================================================================
 */


void PIO_READ_ATA(uint8_t drive, uint32_t start, uint8_t sctrWRITE, uint16_t *buf)
{
    //CAN ONLY HANDLE ONE SECTOR
    
    //check for and error
    if(inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x01) ATA_swreset(); //nuke with software reset
    if(inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x20) ATA_swreset();

    //trace("Drive is: %i\n", (int) drive);
    
    start = start & 0x0FFFFFFF; //make the starting sector val 28 bits
    //trace("Start read sector at: %i\n", (int) start);
    
    if(drive > 1) error(6);
    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | (drive << 4) /*| ((start >> 24) & 0x0F)*/);
    
    //trace("Send drive is: %i\n", (int) 0xE0 | (drive << 4) /*| ((start >> 24) & 0x0F)*/);

    outb(ATA_PRIMARY_DRIVE_FEATURES, 0x00); //no DMA

    outb(ATA_PRIMARY_DRIVE_SCTRCNTP, sctrWRITE); //set # of sectors to read

    //set the starting sector
    outb(ATA_PRIMARY_DRIVE_LBALOWP, (uint8_t) start); 
    outb(ATA_PRIMARY_DRIVE_LBAMIDP, (uint8_t) (start >> 8));
    outb(ATA_PRIMARY_DRIVE_LBAHIP, (uint8_t) (start >> 16));

    outb(ATA_PRIMARY_DRIVE_COMSTAT, 0x20);
   
    while((inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x40) != 0x40);

    //wait for the thing to get ready
    ATAwait();
          
      int i = 0;
    
    while(i < (sctrWRITE << 8))
    {
        buf[i] = inw(ATA_PRIMARY_DRIVE_DATAP);

        while((inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x40) != 0x40);
        i++;
    }
    ATAwait();
    
    if(DRIVE_ERROR_DETECT(buf, 256)) //nuke with software reset
    {
        ATA_swreset();
        PIO_READ_ATA(drive, start, sctrWRITE, buf);
    }
}


void PIO_WRITE_ATA(uint8_t drive, uint32_t start, uint8_t sctrWRITE, uint16_t *buf)
{
    //CAN ONLY HANDLE ONE SECTOR
    //trace("Drive is: %i\n", (int) drive);
    
    start = start & 0x0FFFFFFF; //make the starting sector val 28 bits
    
    //trace("Write sector at: %i\n", (int) start);
    
    if(drive > 1) error(6);
    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | (drive << 4) | ((start >> 24) & 0x0F));
    
    //trace("Send drive is: %i\n", (int) 0xE0 | drive << 4 | ((start >> 24) & 0x0F));

    outb(ATA_PRIMARY_DRIVE_FEATURES, 0x00); //no DMA
    outb(ATA_PRIMARY_DRIVE_SCTRCNTP, sctrWRITE); //set # of sectors to read

    //set the starting sector
    outb(ATA_PRIMARY_DRIVE_LBALOWP, (uint8_t) start); 
    outb(ATA_PRIMARY_DRIVE_LBAMIDP, (uint8_t) (start >> 8));
    outb(ATA_PRIMARY_DRIVE_LBAHIP, (uint8_t) (start >> 16));
    outb(ATA_PRIMARY_DRIVE_COMSTAT, 0x30); //command for read
   
    while((inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x40) != 0x40);

    //wait for the thing to get ready
    ATAwait();
          
      int i = 0;
    while(i < (sctrWRITE << 8))
    {
        outw(ATA_PRIMARY_DRIVE_DATAP, buf[i]);
        ATAwait(); //there should be a short waiting moment
        while((inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x40) != 0x40);
        i++;
    }

    outb(ATA_PRIMARY_DRIVE_COMSTAT, 0xE7);
    ATAwait();
    outb(ATA_PRIMARY_DRIVE_COMSTAT, 0xE7);
}


void PIO_READ_ATAPI(uint8_t drive, uint32_t start, uint32_t sctrWRITE, uint16_t *buf, uint16_t bufSize)
{
    //TODO
    //CAN ONLY HANDLE ONE SECTOR
    uint8_t readcom[12] = {ATAPI_COMMAND_READ,0,0,0,0,0,0,0,0,0,0,0};
    uint8_t status;

    ATA_swreset();
    
    //trace("Drive is: %i\n", (int) drive);
    //trace("Start read sector at: %i\n", (int) start);
    
    if(drive > 1) error(6);
    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | (drive << 4));
    
    //trace("Send drive is: %i\n", (int) 0xE0 | 1 << 4);
    
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);
    inb(ATA_PRIMARY_DRIVE_CONTROLP);

    nIEN_rs(); //clear if set, so that we can receive interrupts

    print("setting drive features and max buffersize...");
    outb(ATA_PRIMARY_DRIVE_FEATURES, 0x00); //no DMA


    outb(ATA_PRIMARY_DRIVE_LBAMIDP,  (2048 & 0x00FF)); //set max size of the thing (lower byte of 512)
    outb(ATA_PRIMARY_DRIVE_LBAHIP, 2048 >> 8);  //same here (upper byte of value 512)
    print("done!\n");

    print("Sending ATAPI packet...");
    outb(ATA_PRIMARY_DRIVE_COMSTAT, 0xA0); //command
    

    //wait for the drive to process the packet command
   while(inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x80) asm volatile("pause"); //pause apparently helps cpu performance
   while(!(status = inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x08) && !(status & 0x1)) 
   asm volatile("pause"); //pause apparently helps cpu performance

    //while((inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x80 == 0x80));

    print(" done!\n");
    
   if(status & 0x01)
    {
        return;
    }

    print("Sending ATAPI command...");

    readcom[9] = 1; //one sector
    readcom[2] = (start >> 0x18);
    readcom[3] = (start >> 0x10);
    readcom[4] = (start >> 0x08);
    readcom[5] = start;


    uint16_t *com = (uint16_t *) readcom;
    uint16_t startlo = (readcom[4] << 8) | readcom[5];
    uint16_t starthi = (readcom[2] << 8) | readcom[3];
    for(uint8_t j; j < 6; j++){
       outw(ATA_PRIMARY_DRIVE_DATAP, com[j]);
   }
    //outsw(ATA_PRIMARY_DRIVE_DATAP, (uint16_t *) readcom, 6);
    /*outw(ATA_PRIMARY_DRIVE_DATAP, ATAPI_COMMAND_READ);
    outw(ATA_PRIMARY_DRIVE_DATAP, starthi);
    outw(ATA_PRIMARY_DRIVE_DATAP, startlo);
    outw(ATA_PRIMARY_DRIVE_DATAP, 0);
    outw(ATA_PRIMARY_DRIVE_DATAP, sctrWRITE);
    outw(ATA_PRIMARY_DRIVE_DATAP, 0);*/

    print("done!\n");


   systeminfo.waitingforATAPI = true;
   //while(systeminfo.waitingforATAPI){ }
  
    int transfersize = inb(ATA_PRIMARY_DRIVE_LBAHIP) << 8 | inb(ATA_PRIMARY_DRIVE_LBAMIDP);
    trace("Transfer size (b): %i\n", transfersize);
    transfersize = transfersize / 2; //divide by two
    trace("Transfer size (w): %i\n", transfersize);
    
    print("fetching data...");
    int i = 0;       

        while(i < transfersize){
            buf[i] = inw(ATA_PRIMARY_DRIVE_DATAP);
            i++;
        }

        if(inb(ATA_PRIMARY_DRIVE_COMSTAT) & 0x88)
        {
            //asm("hlt");
        }
    
    print("done!\n");

    //ATAwait();
    //sleep(300);

}



/* ===================================================================
 *                          DMA STUFF
 * ===================================================================
 */

typedef struct{
    uint32_t *buf;
    uint16_t resv;
    uint16_t transsize; //byte count
    
} tDMA_PRD;

typedef struct{
    tDMA_PRD *prd[2]; //each for one drive
}tDMA_PRDT;

typedef struct{
    uint8_t command;
    uint8_t status;
    uint32_t PRDT_adress;
}tBUSMASTERREG;

tBUSMASTERREG *BMR;
tDMA_PRDT *prdt;

        //pci 0x08 for status, PRDT location and command (bar #4)
        //offset:
        //0x0 -> primary comman
        //0x2 -> primary status
        //0x4-0x7 -> primary PRDT address

        //0x8 -> secondary command
        //0xA -> secondary status
        //0xc-0xf -> secondary PRDT address

        //TODO:
        //* set PRDT location bar 4
        //* set read/write
        //* clear ERR and int (bit 1 and 2 of status)
        //* select drive
        //* send LBA and sector count to its ports
        //* send DMA transfer command to the ATA controller (features bit?)
        //* set start/stop bit
        //* wait for IRQ
        //* clear start/stop bit

void DMA_ATA_SETUP()
{
    uint32_t master_bus = GetPCIDevice(0x01, 0x01, true, true);
    uint8_t bus = getBYTE(master_bus, 2);
    uint8_t dev = getBYTE(master_bus, 1);
    uint8_t fun = getBYTE(master_bus, 0);
    
    BMR = (tBUSMASTERREG *) pciConfigRead(bus, dev, fun, 0x08);
    prdt = malloc(sizeof(tDMA_PRDT));
    
    BMR[0].PRDT_adress = (uint32_t) prdt;

    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | (0 << 4) | ((0 >> 24) & 0x0F));
    outb(ATA_PRIMARY_DRIVE_FEATURES, 0x01); //right?

    //BMR->status |= 0x20;

    outb(ATA_PRIMARY_DRIVE_SCTRCNTP, inb(ATA_PRIMARY_DRIVE_SCTRCNTP) | 0x20);
    //if(BMR->status & 0x20) print("configged\n");
}

void DMA_ATA_READ(uint8_t drive, uint32_t *buffer, uint32_t start, uint8_t nSectors)
{
    
    uint16_t bytecount;
    uint32_t readconfig;
    if(nSectors >= 125) bytecount = 0; //64k is max for DMA
    else bytecount = nSectors * 512;

    prdt->prd[drive]->transsize = bytecount;
    prdt->prd[drive]->buf = buffer;
    
    //clear read/write
    outb(ATA_PRIMARY_DRIVE_DATAP, 0x00);

    //same with ERR and int
    BMR->status &= 0x09;
    
    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | (drive << 4) | ((start >> 24) & 0x0F));
    
    start &= 0x0FFFFFFF;
    outb(ATA_PRIMARY_DRIVE_LBALOWP, (uint8_t) start); 
    outb(ATA_PRIMARY_DRIVE_LBAMIDP, (uint8_t) (start >> 8));
    outb(ATA_PRIMARY_DRIVE_LBAHIP, (uint8_t) (start >> 16));
    outb(ATA_PRIMARY_DRIVE_FEATURES, 0x01); //right?
    
    //set start/stop
    BMR->command &= 0x0;

    //wait for IRQ

    while(1) if(BMR->status & 0x06) print("INT 3\n");
    //clear start/stop
    BMR->command |= 0x0E;
}



void DMA_ATA_WRITE(uint8_t drive, uint32_t *buffer, uint32_t start, uint8_t nSectors)
{
    tDMA_PRDT *prdt = malloc(512 * nSectors);
    uint16_t bytecount;
    

    if(nSectors >= 125) bytecount = 0; //64k is max for DMA
    else bytecount = nSectors * 512;

    
    prdt->prd[drive]->transsize = bytecount;
    prdt->prd[drive]->buf = buffer;
    
    //set read/write
    //same with ERR and int
    
    outb(ATA_PRIMARY_DRIVE_SELECTP, 0xE0 | (drive << 4) | ((start >> 24) & 0x0F));
    
    start &= 0x0FFFFFFF;
    outb(ATA_PRIMARY_DRIVE_LBALOWP, (uint8_t) start); 
    outb(ATA_PRIMARY_DRIVE_LBAMIDP, (uint8_t) (start >> 8));
    outb(ATA_PRIMARY_DRIVE_LBAHIP, (uint8_t) (start >> 16));
    outb(ATA_PRIMARY_DRIVE_FEATURES, 0x01); //right?
    
    //set start/stop
    //wait for IRQ
    //clear start/stop
}

//ATAPI


/* ===================================================================
 *                          INTERRUPTS
 * ===================================================================
 */

void ATA_STANDARD(){
    print("\nATAINT\n");
    int a = inb(ATA_PRIMARY_DRIVE_COMSTAT);
    systeminfo.waitingforATAPI = false;
    outb(PIC1,0x20);
	outb(PIC2,0x20);
}
