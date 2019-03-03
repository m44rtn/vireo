#ifndef MULTITASKING_H
#define MULTITASKING_H

#include "../include/types.h"
#include "../include/info.h"
#include "../drivers/cpu.h"
#include "memory.h"

#define TASK_HIGH   0x01
#define TASK_MID    0x02
#define TASK_LOW    0x03

#define TASK_FLAG_v86 0b01
#define TASK_FLAG_KERNEL 0b10

typedef struct 
{
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;

    uint32_t esp;
    uint32_t ebp;

    uint32_t esi;
    uint32_t edi;
} __attribute__ ((packed)) REGISTERS;


typedef struct{
    //serves as eip
    uint32_t entry_ptr;

    uint8_t quantum;

    uint8_t priority;
    uint16_t flags;

    REGISTERS registers;
} __attribute__ ((packed)) tTask; 


void task_push(uint8_t priority, uint32_t entry_point, uint16_t flags);
void task_push_v86(uint32_t ebp, uint32_t entry_point, uint8_t priority, uint16_t flags);



#endif