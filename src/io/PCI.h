#ifndef PCI_H
#define PCI_H

#include "sys.h"
#include "../include/types.h"
#include "../include/info.h"
#include "../include/error.h"
#include "../io/util.h"



#define GETBYTE false
#define GETWORD true


//void pciCheckDevice(uint16_t bus, uint8_t device);
//void pciCheckBus(uint8_t bus);
//void pciCheckFunc(uint8_t bus, uint8_t device, uint8_t func);
void PCIsetup();

int pciAmountOfDevices();
uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg);
uint16_t getPCIval(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint8_t strt, bool BorW);

uint8_t pciGetInterruptLine(uint8_t bus, uint8_t device, uint8_t func);
void pciConfigWrite(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val);

uint32_t GetPCIDevice(uint8_t basecl, uint8_t subcl, bool iTrace, bool iErr);


void pciCheckDevice(uint16_t bus, uint8_t device);
void checkfunc(uint16_t bus, uint8_t device, uint8_t function);

typedef struct tagdevinf{
    uint8_t MF:1;
    uint8_t CL;
    uint8_t SCL;
    uint8_t HT;
    uint16_t bus;
    uint8_t device;
    uint8_t func;
} DeviceInfo;


#endif