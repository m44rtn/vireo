#ifndef ISR_H
#define ISR_H

#include "../include/types.h"
#include "../drivers/cpu.h"
#include "hardware.h"
#include "../drivers/keyboard.h"
#include "../include/info.h"
#include "../drivers/ATA/ATA.h"
//#include "../drivers/v86.h"

#define PIC1      0x20
#define PIC2      0xA0

void isrinst();
extern void isr0();
extern void isr1();
extern void isr3();
extern void isr4();
extern void isr7();
extern void isr8();
extern void isr10();
extern void isr11();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr20();
extern void isr21();

extern void isr30();
extern void isr47();

void isr0c();
void isr1c();
void isr3c(uint32_t eax);
void isr4c();
void isr7c();
void isr8c();
void isr11c();
void isr12c();
void isr13c(uint16_t ss, uint32_t esp, uint32_t eflags, uint16_t cs, uint16_t ip/*uint16_t ip, uint16_t cs, uint32_t eflags, uint32_t esp, uint16_t ss*/);
void isr15c();
void isr20c();
void isr21c();
void isr2ec();
void isr2fc();
void isr30c();
void isr47c();

extern void PIT_readcount(uint16_t countval);

#endif
