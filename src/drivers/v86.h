#ifndef V86_H
#define V86_H

#include "../io/multitasking.h"
#include "DriverHandler.h"
#include "../io/memory.h"

void v86_save_state(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,  uint32_t eip, uint32_t cs);
uint32_t v86_linear_to_sgoff(uint32_t ptr);
uint32_t v86_sgoff_to_linear(uint16_t segment, uint16_t offset);
void v86_interrupt(uint16_t interrupt, tREGISTERS *registers);
extern void v86_enter(uint32_t *vtask, uint32_t *task_registers);

#endif