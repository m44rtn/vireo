/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

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
#include "../COMMANDS.H"

#include "../../include/exit_code.h"
#include "../../include/types.h"
#include "../../dsk/diskdefines.h"

#include "../../cpu/interrupts/IDT.h"
#include "../../hardware/pic.h"

#ifndef NO_DEBUG_INFO
#include "../../screen/screen_basic.h"
#endif

#include "../../dbg/dbg.h"

#include "../../hardware/pci.h"
#include "../../hardware/driver.h"

#include "../../memory/memory.h"

#include "../../io/io.h"

#include "../../dbg/dbg.h"

#include "../../util/util.h"

#define IDEController_PCI_CLASS_SUBCLASS    0x101

#define IDE_DRIVER_VERSION_STRING "[IDE_DRIVER] Vireo Internal PIO IDE/ATA Driver\n"

#define DEFAULT_SECTOR_SIZE         512     // bytes
#define DEFAULT_ATAPI_SECTOR_SIZE   2048    // bytes

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
#define ATA_STAT_READY  0x40
#define ATA_STAT_BUSY   0x80

#define ATAPI_COMMAND_READ      0xA8

#define ATA_COMMAND_MAX_ADDR    0xF8

#define ATA_COMMAND_READ        0x20
#define ATA_COMMAND_WRITE       0x30
#define ATA_COMMAND_CACHE_FLUSH 0xE7

#define ATAPI_COMMAND_PACKET    0xA0
#define ATAPI_COMMAND_READ      0xA8

#define ATA_COMMAND_DMAREAD     0xC8
#define ATA_COMMAND_DMAWRITE    0xCA

#define ATAPI_IDENTIFY          0xA1
#define ATA_IDENTIFY            0xEC

/* Flags stuff */
#define IDE_FLAG_INIT_RAN   1 /* used by init to say it did ran and did it's thing */
#define IDE_FLAG_IRQ        1 << 2


typedef struct
{
    uint8_t type;
    /* there'll be more here, probably */
} DRIVE_INFO;

/* functions defined here, because it *should* be private to the driver */

void IDEController_handler(uint32_t *drv);

extern void ASM_IDE_IRQ(void);

void IDE_IRQ(void);

static void IDE_software_reset(uint16_t port);
static void IDE_wait(void);
static uint8_t IDE_polling(uint16_t port, bool errTest);
static uint16_t IDE_getPort(uint8_t drive);
static uint8_t IDE_getSlavebit(uint8_t drive);
static void IDEClearFlagBit(uint16_t flag_bit);

static void IDEDriverInit(unsigned int device);
#ifndef NO_DEBUG_INFO
static void IDEPrintWelcome(void);
#endif
static void IDE_enumerate(void);
static uint8_t IDE_getDriveType(uint16_t port, uint8_t slavebit);

static uint8_t IDE_readPIO28(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf);
static uint8_t IDE_writePIO28(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf);
static uint8_t IDE_readPIO28_atapi(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf);

static void IDE_reportDrives(uint8_t *drive_list);

DRIVE_INFO drive_info_t[IDE_DRIVER_MAX_DRIVES];
uint32_t PCI_controller;

uint16_t p_base_port;
uint16_t p_ctrl_port;
uint16_t s_base_port;
uint16_t s_ctrl_port;

/* some flag values:
        - bit 0: if set, init executed succesfully
        - bit 1: IRQ fired
        */
uint16_t ide_flags = 0;

/* the indentifier for drivers + information about our driver */
struct DRIVER IDE_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (IDEController_PCI_CLASS_SUBCLASS | DRIVER_TYPE_PCI), (uint32_t) (IDEController_handler)};

// FIXME remove static declarations/prototypes and put all functions in .h

static uint32_t ide_get_max_addr(uint8_t drive)
{
    uint16_t port = IDE_getPort(drive);
    
    outb(port | ATA_PORT_COMSTAT, ATA_COMMAND_MAX_ADDR);

    IDE_polling(port, false);

    uint32_t lba = (uint32_t) inb(port | ATA_PORT_LBALOW);
    lba += (uint32_t) inb(port | ATA_PORT_LBAMID) << 8;
    lba += (uint32_t) inb(port | ATA_PORT_LBAHI) << 16;
    lba += (uint32_t) (inb(port | ATA_PORT_SELECT) & 0x0F) << 24;
    
    return lba;
}

void IDEController_handler(uint32_t *drv)
{
    uint8_t error = 0;

    /* To make sure that we don't do anything stupid, we check if INIT is either being called NOW
or has executed succesfully in the past */
    if(drv[0] != DRV_COMMAND_INIT && flag_check(ide_flags, IDE_FLAG_INIT_RAN))
        return;

    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
            if(!flag_check(ide_flags, IDE_FLAG_INIT_RAN))
                break;

            IDEDriverInit(drv[1]);
        break;

        case IDE_COMMAND_READ:
            if(drive_info_t[drv[1]].type == DRIVE_TYPE_IDE_PATA)
                error = IDE_readPIO28((uint8_t) drv[1], drv[2], (uint8_t) drv[3], (uint16_t *) drv[4]);
            if(drive_info_t[drv[1]].type == DRIVE_TYPE_IDE_PATAPI)
                error = IDE_readPIO28_atapi((uint8_t) drv[1], drv[2], (uint8_t) drv[3], (uint16_t *) drv[4]);
        break;

        case IDE_COMMAND_WRITE:
            if(drive_info_t[drv[1]].type != DRIVE_TYPE_IDE_PATA)
            {
                error = EXIT_CODE_GLOBAL_GENERAL_FAIL;
                break;
            }

            IDE_writePIO28((uint8_t) drv[1], drv[2], (uint8_t) drv[3], (uint16_t *) drv[4]);
        break;

        case IDE_COMMAND_REPORTDRIVES:
            if(drv[1])
                IDE_reportDrives((uint8_t *) *(&drv[1]));
            else
                error = EXIT_CODE_GLOBAL_OUT_OF_RANGE;
        break;

        case IDE_COMMAND_GET_MAX_ADDRESS:
            drv[2] = ide_get_max_addr((uint8_t) drv[1]);
        break;

        default:
            error = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
    }

    if(!error)
        return;

    /* else */    
    drv[4] = NULL;
    drv[1] = error;
    
}

// ISR
void IDE_IRQ(void)
{
    ide_flags = ide_flags | IDE_FLAG_IRQ;

    PIC_EOI(0x0F);
}

static void IDE_software_reset(uint16_t port){
    uint8_t value = (uint8_t) inb(port);
    outb(port, (value | 0x04));

    sleep(5);

    /* unset the reset bit */
    value = (uint8_t) inb(port);
    outb(port, ((uint8_t)(value & 0xFFFBU)));
}

static void IDE_wait(void)
{
    if(pciGetReg0(PCI_controller) == 0x24CB8086)
    { sleep(4); return; }

    inb(p_ctrl_port);
    inb(p_ctrl_port);
    inb(p_ctrl_port);
    inb(p_ctrl_port);
}

static uint8_t IDE_polling(uint16_t port, bool errTest)
{
    uint8_t status;

    IDE_wait();

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

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static uint16_t IDE_getPort(uint8_t drive)
{
    return (drive > 1) ? s_base_port : p_base_port;
}

static uint8_t IDE_getSlavebit(uint8_t drive)
{
    return (drive % 2) ? (uint8_t) 1 : 0;
}

static void IDEClearFlagBit(uint16_t flag_bit)
{
    ide_flags = ide_flags & (uint16_t)~(flag_bit);
}

static void IDEDriverInit(uint32_t device)
{
    uint8_t drive, slavebit;
    uint16_t port;

    PCI_controller = device & (uint32_t)~(DRIVER_TYPE_PCI);

    /* get the ports for both primary and secondary */
    IDE_enumerate();
    
    IDE_software_reset(p_ctrl_port);
    IDE_software_reset(s_ctrl_port);

    /* register our IRQ handler */
    IDT_add_handler(0x2E, (uint32_t) ASM_IDE_IRQ);
    IDT_add_handler(0x2F, (uint32_t) ASM_IDE_IRQ);

    /* disable IRQs */
    outb((uint32_t) p_ctrl_port, 2);
    outb((uint32_t) s_ctrl_port, 2);

    /* get the drive types */
    for(drive = 0; drive < IDE_DRIVER_MAX_DRIVES; ++drive)
    {
        port = IDE_getPort(drive);
        slavebit = IDE_getSlavebit(drive);

        drive_info_t[drive].type = IDE_getDriveType(port, slavebit);
    }

    outb((uint32_t) p_ctrl_port, 0);
    outb((uint32_t) s_ctrl_port, 0);

    /* set the flag 'INIT ran successfully' */
    ide_flags = ide_flags | IDE_FLAG_INIT_RAN;
    
#ifndef NO_DEBUG_INFO
    IDEPrintWelcome();
#endif

}
#ifndef NO_DEBUG_INFO
static void IDEPrintWelcome(void)
{
    print( IDE_DRIVER_VERSION_STRING);
    print_value( "[IDE_DRIVER] Kernel reported PCI controller %x\n", PCI_controller);

    print_value( "[IDE_DRIVER] Primary base port: %x\n", p_base_port);
    print_value( "[IDE_DRIVER] Secondary base port: %x\n", s_base_port);
    print_value( "[IDE_DRIVER] Primary control port: %x\n", p_ctrl_port);
    print_value( "[IDE_DRIVER] Secondary control port: %x\n", s_ctrl_port);

    print( "\n");

}
#endif

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
    
    // TODO: check whether in Legacy or Native mode before doing this
    if(pciGetReg0(PCI_controller) == 0x24CB8086)
    {
        // this controller (8086:24cb) does not seem to report its control block correctly.
        // According to the datasheet these are 0x3f4 and 0x374 respectively
        p_ctrl_port = 0x3f4;
        s_ctrl_port = 0x374;
    }

}

static uint8_t IDE_getDriveType(uint16_t port, uint8_t slavebit)
{
    uint16_t status = 0;
    uint16_t *buffer;
    uint16_t lo, hi;
    uint8_t type = DRIVE_TYPE_IDE_PATA;

    uint16_t port_comstat = port | ATA_PORT_COMSTAT;

    outb((uint32_t) (port | ATA_PORT_SELECT), ((uint8_t)0xA0U) | (uint8_t)((slavebit) << 4U));

    IDE_polling(port, false);

    /* apparently, when sending ATA_IDENTIFY to an ATAPI device virtualbox raises a general protection fault.
       to avoid the #GP we first check if we're dealing with a PATAPI device */
    lo = inb( (port | ATA_PORT_LBAMID)) & 0xFF;
    hi = inb( (port | ATA_PORT_LBAHI)) & 0xFF;

    if(hi == 0xEB && lo == 0x14)
        type = DRIVE_TYPE_IDE_PATAPI;

    /* send the IDENTIFY command in case of PATA (not connected is also hi == lo == 0) */
    if(type == DRIVE_TYPE_IDE_PATA) outb((uint32_t) port_comstat, ATA_IDENTIFY);
    else if(type == DRIVE_TYPE_IDE_PATAPI)
        return type;

   IDE_polling(port, false);

    /* if status = 0 then there's no such device */
    uint8_t stat = inb(port_comstat);
    if(!stat || stat == 0x7F || stat == 0x01)
        return (uint8_t) DRIVE_TYPE_UNKNOWN;

    /* wait until BSY clears */
    while(inb(port_comstat) & 0x80);

    /* wait for DRQ and/or ERR sets*/
    while(1)
    {
        status = (uint16_t) inb(port_comstat);
        if(status & 0x01) break;
        if(status & 0x08) break;
    }

    // check for the revised lo and hi which can better indicate the
    // device type.
    lo = inb( (port | ATA_PORT_LBAMID)) & 0xFF;
    hi = inb( (port | ATA_PORT_LBAHI)) & 0xFF;

    if(hi == 0xEB && lo == 0x14)
        return DRIVE_TYPE_IDE_PATAPI;
    else if(hi == 0x7F && lo == 0x7F)
        return DRIVE_TYPE_UNKNOWN;

    /* right now we discard the info returned by the device, though it may be useful to not discard this in the future */
    buffer = kmalloc(256 * sizeof(uint16_t));

    insw(port, 255, buffer);

    kfree(buffer);

    return type;
}

static uint8_t IDE_readPIO28(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf)
{
    uint8_t i = 0;
    uint16_t port = IDE_getPort(drive);
    uint8_t slavebit = IDE_getSlavebit(drive);
    uint8_t *buf_ptr = (uint8_t *) buf;

    while((inb(port |ATA_PORT_COMSTAT) & ATA_STAT_BUSY));
    
    if(drive > 3)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;
    if(drive_info_t[drive].type != DRIVE_TYPE_IDE_PATA)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;

    outb(port | ATA_PORT_SELECT,  ((uint8_t)0xE0U) | ((uint8_t)(slavebit << 4U)) | ((((uint8_t)start >> 24U)) & 0x0F));

    outb(port | ATA_PORT_FEATURES, 0U); /* no DMA */
    outb(port | ATA_PORT_SCTRCNT, sctrwrite);

    start = start & 0x0FFFFFFFU;

    outb(port | ATA_PORT_LBALOW, (uint8_t) start);
    outb(port | ATA_PORT_LBAMID, (uint8_t) (start >> 8U));
    outb(port | ATA_PORT_LBAHI, (uint8_t) (start >> 16U));

    outb(port | ATA_PORT_COMSTAT, ATA_COMMAND_READ);

    if(IDE_polling(port, false))
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;

    for(i = 0; i < sctrwrite; ++i)
    {
        IDE_polling(port, false);
        while(!(inb(port | ATA_PORT_COMSTAT) & ATA_STAT_READY));

        insw(port, 256, (uint16_t *) buf_ptr);

        buf_ptr += DEFAULT_SECTOR_SIZE;
    }

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static uint8_t IDE_writePIO28(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf)
{
    uint8_t i = 0;
    uint16_t port = IDE_getPort(drive);
    uint8_t slavebit = IDE_getSlavebit(drive);
    uint8_t *buf_ptr = (uint8_t *) buf;

    while((inb(port |ATA_PORT_COMSTAT) & ATA_STAT_BUSY));

    if(drive > 3)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;
    if(drive_info_t[drive].type != DRIVE_TYPE_IDE_PATA)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;

    outb(port | ATA_PORT_SELECT,  ((uint8_t)0xE0U) | ((uint8_t)(slavebit << 4U)) | ((((uint8_t)start >> 24U)) & 0x0F));

    outb(port | ATA_PORT_FEATURES, 0U); /* no DMA */
    outb(port | ATA_PORT_SCTRCNT, sctrwrite);

    start = start & 0x0FFFFFFFU;

    outb(port | ATA_PORT_LBALOW, (uint8_t) start);
    outb(port | ATA_PORT_LBAMID, (uint8_t) (start >> 8U));
    outb(port | ATA_PORT_LBAHI, (uint8_t) (start >> 16U));

    outb(port | ATA_PORT_COMSTAT, ATA_COMMAND_WRITE);
    IDE_wait();

    while(!(inb(port |ATA_PORT_COMSTAT) & 0x40));
    
    for(i = 0; i < sctrwrite; ++i)
    {
        IDE_polling(port, false);
        while(!(inb(port | ATA_PORT_COMSTAT) & ATA_STAT_READY));

        outsw(port, 256, (uint16_t *) buf_ptr);
        
        buf_ptr += DEFAULT_SECTOR_SIZE;
    }
    IDE_polling(port, false);
    while(!(inb(port | ATA_PORT_COMSTAT) & ATA_STAT_READY));
    outb(port | ATA_PORT_COMSTAT, ATA_COMMAND_CACHE_FLUSH);
    IDE_polling(port, false);

    return EXIT_CODE_GLOBAL_SUCCESS;
}

static uint8_t IDE_readPIO28_atapi(uint8_t drive, uint32_t start, uint8_t sctrwrite, uint16_t *buf)
{
    uint8_t read_command[12] = {ATAPI_COMMAND_READ,0,0,0,0,0,0,0,0,0,0};
    uint16_t port = IDE_getPort(drive);
    uint8_t slavebit = IDE_getSlavebit(drive);
    uint8_t status;

    if(drive > 3)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;
    if(drive_info_t[drive].type != DRIVE_TYPE_IDE_PATAPI)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;

    IDEClearFlagBit(IDE_FLAG_IRQ);

    outb(port | ATA_PORT_SELECT,  ((uint8_t)0xE0U) | ((uint8_t)(slavebit << 4U)) | ((((uint8_t)start >> 24U)) & 0x0F));

    outb(port | ATA_PORT_FEATURES, 0U);
    outb(port | ATA_PORT_LBAMID, (uint8_t) (DEFAULT_ATAPI_SECTOR_SIZE & 0xFF));
    outb(port | ATA_PORT_LBAHI, (uint8_t) (DEFAULT_ATAPI_SECTOR_SIZE >> 8U));

    outb(port | ATA_PORT_COMSTAT, ATAPI_COMMAND_PACKET);
  
    while(inb(port | ATA_PORT_COMSTAT) & 0x80) __asm__ __volatile__("pause");
    while(!(status = inb(port | ATA_PORT_COMSTAT) & ATA_STAT_DRQ)) 
        __asm__ __volatile__("pause");

    /* error */
    if(status & 0x01)
        return EXIT_CODE_IDE_ERROR_READING_DRIVE;
    
    read_command[2] = (uint8_t) (start >> 0x18) & 0xFF;
    read_command[3] = (uint8_t) (start >> 0x10) & 0xFF;
    read_command[4] = (uint8_t) (start >> 0x08) & 0xFF;
    read_command[5] = (uint8_t) (start >> 0x00) & 0xFF;
    read_command[9] = (uint8_t) sctrwrite;

    outsw(port, 6, (uint16_t *) &read_command);

    while(!(ide_flags & IDE_FLAG_IRQ))
            __asm__ __volatile__("pause");
    IDEClearFlagBit(IDE_FLAG_IRQ);
    
    uint32_t size = (uint32_t)(inb(port | ATA_PORT_LBAHI) << 8U) | inb(port | ATA_PORT_LBAMID);
    uint32_t nwords = size / 2;
    uint32_t counter = 0;

    while ((inb(port | ATA_PORT_COMSTAT) & (ATA_STAT_BUSY | ATA_STAT_DRQ)))
    {
        insw(port, (uint32_t)nwords, &buf[counter]);
        counter = counter + nwords;
     
        while (!(ide_flags & IDE_FLAG_IRQ))
            __asm__ __volatile__("pause");
        IDEClearFlagBit(IDE_FLAG_IRQ);
    }
        
    return EXIT_CODE_GLOBAL_SUCCESS;
}

static void IDE_reportDrives(uint8_t *drive_list)
{
    uint32_t i = 0;

    for(; i < IDE_DRIVER_MAX_DRIVES; ++i)
        drive_list[i] = drive_info_t[i].type;
}
