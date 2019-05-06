#ifndef HARDWARE_H
#define HARDWARE_H

#include "util.h"
#include "isr.h"
#include "sys.h"
#include "../drivers/screen/screen.h"



char getscancode();
void setints();
void IRQclrmsk(uint8_t line);
void IRQclrallmsks();
void IRQdisableall();
void IRQmskall();
void IRQsetmsk(uint8_t line);
void initpics(int pic1, int pic2);
void inthndld(unsigned char irq);
void PIC_remap(int offset1, int offset2);
void PIT_init();
void DetectSpeed();
extern void PIT_initasm();

void inita20(void);
void keybsndcom(uint8_t com);
void keybwait();


#endif
