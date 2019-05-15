#ifndef PCI_H
#define PCI_H

#include "sys.h"
#include "../include/types.h"
#include "../include/info.h"
#include "../include/error.h"
#include "../io/util.h"

#define GETBYTE false
#define GETWORD true

typedef struct
{
    uint8_t bus;
    uint8_t device;
    uint8_t func;
    uint8_t header_type;
    uint8_t class;
    uint8_t subclass;
} tPCI_DEVICE;

tPCI_DEVICE pci_devices[256];

void pci_init();
uint32_t pci_get_device(uint8_t class, uint8_t subclass);

uint32_t pciConfigRead (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg);
uint16_t pci_get_val(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint8_t strt, bool BorW);

uint8_t pciGetInterruptLine(uint8_t bus, uint8_t device, uint8_t func);
void pciConfigWrite(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val);

uint32_t GetPCIDevice(uint8_t basecl, uint8_t subcl, bool iTrace, bool iErr);

#endif