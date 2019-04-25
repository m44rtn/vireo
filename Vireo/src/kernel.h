#ifndef KERNEL_H
#define KERNEL_H

#include "include/types.h"
#include "drivers/screen.h"
#include "io/gdt.h"
#include "drivers/cpu.h"
#include "io/isr.h"
#include "io/hardware.h"
#include "io/util.h"
#include "drivers/keyboard.h"
#include "io/memory.h"
#include "include/info.h"
#include "include/keych.h"
#include "include/error.h"
#include "io/PCI.h"
#include "drivers/ATA/SATA.h"
#include "drivers/ATA/ATA.h"
#include "drivers/FS/FAT.h"
#include "drivers/FS/ISO9660.h"
#include "drivers/HDD.h"
#include "drivers/DriverHandler.h"
#include "drivers/v86.h"
#include "io/multitasking.h"
#include "include/DEFATA.h"
#include "include/GRUB/multiboot.h" //mutliboot stuff --> grub


void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs);
void setsysinf();
void ver(char* verinf);

void main_set_flags(uint32_t flags);

extern void halt();

int GRUBMAGIC = 0x1BADB002;

#endif
