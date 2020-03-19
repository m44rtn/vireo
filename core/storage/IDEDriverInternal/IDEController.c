/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "IDEController.h"

#include "../IDE_commands.h"

#include "../../include/exit_code.h"
#include "../../include/types.h"

#ifndef QUIET_KERNEL
    #include "../../screen/screen_basic.h"
#endif

#include "../../hardware/pci.h"
#include "../../hardware/driver.h"

#include "../../memory/memory.h"

#include "../../io/io.h"

#include "../../dbg/dbg.h"

#include "../../util/util.h"

#define IDEController_PCI_CLASS_SUBCLASS    0x101

/* defines for ata_info_t */
#define ATA_INFO_PRIMARY    0x00
#define ATA_INFO_SECONDARY  0x01

#define ATA_PORT_FEATURES   0x01
#define ATA_PORT_SCTRCNT    0x02
#define ATA_PORT_LBALOW     0x03
#define ATA_PORT_LBAMID     0x04
#define ATA_PORT_LBAHI      0x05
#define ATA_PORT_SELECT     0x06
#define ATA_PORT_COMSTAT    0x07

#define ATA_STAT_ERR    0x01
#define ATA_STAT_DRQ    0x08
#define ATA_STAT_DF     0x20
#define ATA_STAT_BUSY   0x80

#define ATAPI_COMMAND_READ 0xA8

#define ATA_COMMAND_READ    0x20
#define ATA_COMMAND_WRITE   0x30

#define ATA_COMMAND_DMAREAD 0xC8
#define ATA_COMMAND_DMAWRITE 0xCA

#define ATAPI_IDENTIFY   0xA1
#define ATA_IDENTIFY     0xEC

#define IDE_DRIVER_MAX_DRIVES   4

#define IDE_DRIVER_TYPE_PATA    0x00
#define IDE_DRIVER_TYPE_PATAPI  0x01
#define IDE_DRIVER_TYPE_UNKNOWN 0xFF


typedef struct
{
    uint8_t type;
    /* there'll be more here, probably */
} DRIVE_INFO;


static void IDE_software_reset(uint16_t port);
static void IDE_wait(uint16_t port);
static uint8_t IDE_polling(uint16_t port, bool errTest);
static uint16_t IDE_getPort(uint8_t drive);

static void IDEDriverInit(unsigned int device);
static void IDE_enumerate(void);
static uint8_t IDE_getDriveType(uint16_t port, uint8_t slavebit);

static void IDE_readPIO28(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf);

DRIVE_INFO drive_info_t[IDE_DRIVER_MAX_DRIVES];
uint32_t PCI_controller;

uint16_t p_base_port;
uint16_t p_ctrl_port;
uint16_t s_base_port;
uint16_t s_ctrl_port;

/* some flag values:
        - bit 0: if set, init executed succesfully
        */    
uint16_t flags = 0;

/* the indentifier for drivers + information about our driver */
struct DRIVER driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (IDEController_PCI_CLASS_SUBCLASS | DRIVER_TYPE_PCI), (uint32_t) (IDEController_handler)};


void IDEController_handler(uint32_t *drv)
{
    trace( (char*)"command=%x\n", drv[0]);
    
    /* To make sure that we don't do anything stupid, we check if INIT is either called or previously
        executed */
    if(drv[0] != IDE_COMMAND_INIT && flag_check(flags, 1U))
        return;

    switch(drv[0])
    {
        case IDE_COMMAND_INIT:
            IDEDriverInit(drv[1]);
        break;

        case IDE_COMMAND_READ:
            IDE_readPIO28((uint8_t) drv[1], drv[2], (uint8_t) drv[3], (uint16_t *) drv[4]);
        break;

        default:
            dbg_assert(0);
        break;
    }
}

static void IDE_software_reset(uint16_t port){
    uint8_t value = (uint8_t) inb(port);
    outb(port, (value | 0x04));
  
    IDE_wait(port);

    /* unset the reset bit */
    value = (uint8_t) inb(port);
    outb(port, ((uint8_t)(value & 0xFFFBU)));
}

static void IDE_wait(uint16_t port)
{
    
    if(port != p_base_port) 
        port = s_ctrl_port;
    else
        port = p_ctrl_port;
    
    inb(port);
    inb(port);
    inb(port);
    inb(port);
}

static uint8_t IDE_polling(uint16_t port, bool errTest)
{
    uint8_t status;

    IDE_wait(port);

    while(inb((uint16_t) port | ATA_PORT_COMSTAT) & ATA_STAT_BUSY);

    status = (uint8_t) (inb((uint16_t) port | ATA_PORT_COMSTAT) & 0xFF);

    /* Error and Device Fault should be unset, DRQ set */
    if(errTest)
    {
        if (status & ATA_STAT_ERR) 
            return EXIT_CODE_GLOBAL_GENERAL_FAIL;
        if (status & ATA_STAT_DF)
            return EXIT_CODE_GLOBAL_GENERAL_FAIL;
        if (!(status & ATA_STAT_DRQ))
            return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    }

    return 0;
}

static uint16_t IDE_getPort(uint8_t drive)
{
    return (drive > 1) ? s_base_port : p_base_port;
} 

static uint8_t IDE_getSlavebit(uint8_t drive)
{
    return (drive % 2) ? (uint8_t) 1 : 0;
}

static void IDEDriverInit(uint32_t device)
{
    /* technically, you should first enumerate the PCI bus but it's not reliable and 
    most controllers support the standard IO ports at boot up anyway. */
    uint8_t drive, slavebit;
    uint16_t port;
    
    #ifndef QUIET_KERNEL
    print((char *) "[IDE_DRIVER] Vireo Internal PIO IDE/ATA Driver Mk. I\n");
    trace((char *) "[IDE_DRIVER] Kernel reported PCI controller %x\n", device);
    #endif

    PCI_controller = device & (uint32_t)~(DRIVER_TYPE_PCI);

    /* get the ports for both primary and secondary */
    IDE_enumerate();
    
    IDE_software_reset(p_ctrl_port);
    IDE_software_reset(s_ctrl_port);

    /* disable IRQs */
    outb((uint32_t) p_base_port, 2);
    outb((uint32_t) p_base_port, 2);

    /* get the drive types */
    for(drive = 0; drive < IDE_DRIVER_MAX_DRIVES; ++drive)
    {
        port = IDE_getPort(drive);
        slavebit = IDE_getSlavebit(drive);

        /* TODO: 
            - cleanup 
            - ATAPI READ
            - ATA READ/WRITE?*/
        
        drive_info_t[drive].type = IDE_getDriveType(port, slavebit);
        trace((char *) "[IDE_DRIVER] found drive type: %x\n", drive_info_t[drive].type);
    }
    
    #ifndef QUIET_KERNEL
        print((char *) "\n");
    #endif

    flags = flags | 1U;
}

static void IDE_enumerate(void)
{
    uint32_t bar;

    /* get the primary ports... */
    bar = pciGetBar(PCI_controller, PCI_BAR0) & 0xFFFFFFFC;
    p_base_port = (uint16_t) (bar + 0x1F0U*(!bar)) & 0xFFFFU;
    
    bar = pciGetBar(PCI_controller, PCI_BAR1) & 0xFFFFFFFC;
    p_ctrl_port = (uint16_t) (bar + 0x3F6U*(!bar)) & 0xFFFFU; 

    /* ...and the secondary */
    bar = pciGetBar(PCI_controller, PCI_BAR2) & 0xFFFFFFFC;
    s_base_port = (uint16_t) (bar + 0x170U*(!bar)) & 0xFFFFU;
    
    bar = pciGetBar(PCI_controller, PCI_BAR3) & 0xFFFFFFFC;
    s_ctrl_port = (uint16_t) (bar + 0x376U*(!bar)) & 0xFFFFU;

    #ifndef QUIET_KERNEL
    trace((char *) "[IDE_DRIVER] Primary base port: %x\n", p_base_port);
    trace((char *) "[IDE_DRIVER] Secondary base port: %x\n", s_base_port);
    trace((char *) "[IDE_DRIVER] Primary control port: %x\n", p_ctrl_port);
    trace((char *) "[IDE_DRIVER] Secondary control port: %x\n", s_ctrl_port);
    #endif

}

static uint8_t IDE_getDriveType(uint16_t port, uint8_t slavebit)
{
    /* use IDENTIFY */ 
    uint16_t status = 0;
    uint16_t *buffer;
    uint16_t lo, hi;
    uint8_t type = IDE_DRIVER_TYPE_PATA;

    uint16_t port_comstat = port | ATA_PORT_COMSTAT;

    outb((uint32_t) (port | ATA_PORT_SELECT), ((uint8_t)0xA0U) | (uint8_t)((slavebit) << 4U));
    

    /* apparently, when sending ATA_IDENTIFY to an ATAPI device virtualbox raises a general protection fault.
       this is not documented anywhere on wiki.osdev.org, so this may be virtualbox specific.
       to avoid the #GP we first check if we're dealing with a PATAPI or PATA device */
    
    lo = inb( (port | ATA_PORT_LBAMID)) & 0xFF;
    hi = inb( (port | ATA_PORT_LBAHI)) & 0xFF;
    
    if(hi == 0xEB && lo == 0x14) 
        type = IDE_DRIVER_TYPE_PATAPI;

    else if(hi == 0x96 && lo == 0x69) 
        type = IDE_DRIVER_TYPE_PATAPI;

    else if(!(hi == 0 && lo == 0)) 
        return IDE_DRIVER_TYPE_UNKNOWN; 
        

    /* send the IDENTIFY command */
    if(type == IDE_DRIVER_TYPE_PATA) outb((uint32_t) port_comstat, ATA_IDENTIFY);
    else if(type == IDE_DRIVER_TYPE_PATAPI) 
        return type;
    
    /* if status = 0 then there's no such device */
    if(!inb(port_comstat))
        return (uint8_t) IDE_DRIVER_TYPE_UNKNOWN;
    
    /* wait until BSY clears */
    while(inb(port_comstat) & 0x80); 
        
    /* wait for DRQ and/or ERR sets*/
    while(1)
    {
        status = (uint16_t) inb(port_comstat);
        if(status & 0x01) break;
        if(status & 0x08) break;
    }
 
    /* right now we discard the info returned by the device, though it may be useful to not discard this in the future */
    buffer = malloc(256 * sizeof(uint16_t));
    
    insw(port, 255, buffer);
    
    demalloc(buffer);

    return type;
}

static void IDE_readPIO28(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf)
{
    uint8_t i = 0;
    uint16_t port = IDE_getPort(drive);
    uint8_t slavebit = IDE_getSlavebit(drive);

    trace( (char*)"drive: %x\n", drive);
    trace( (char*)"start: %x\n", start);
    trace( (char*)"sctrwrite: %x\n", sctrwrite);
    trace( (char*)"buf: %x\n", (uint32_t) buf);

    if(drive > 3)
        return;
    if(drive_info_t[drive].type != IDE_DRIVER_TYPE_PATA)
        return;

    outb(port | ATA_PORT_SELECT,  ((uint8_t)0xE0U) | ((uint8_t)(slavebit << 4U)) | ((((uint8_t)start >> 24U)) & 0x0F));

    outb(port | ATA_PORT_FEATURES, 0U); /* no DMA */
    outb(port | ATA_PORT_SCTRCNT, sctrwrite);

    start = start & 0x0FFFFFFFU;

    outb(port | ATA_PORT_LBALOW, (uint8_t) start);
    outb(port | ATA_PORT_LBAMID, (uint8_t) (start >> 8U));
    outb(port | ATA_PORT_LBAHI, (uint8_t) (start >> 16U));

    outb(port | ATA_PORT_COMSTAT, ATA_COMMAND_READ);
    
    if(IDE_polling(port, false))
            print( (char*)"HELLOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");

    for(i = 0; i < sctrwrite; ++i)
    {
        insw(port, 256, buf);
        while(!(inb(port |ATA_PORT_COMSTAT) & 0x40));
    }
}
