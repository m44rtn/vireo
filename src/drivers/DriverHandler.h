#ifndef DRIVERHANDLER_H
#define DRIVERHANDLER_H

#include "../include/types.h"
#include "FS/FAT.h"
#include "../include/DriverTypes.h"
#include "../io/util.h"
#include "../io/multitasking.h"
#include "../io/v86.h"

typedef struct{
    uint8_t jmp, shrt, nop;
    char Signature[11];
    uint8_t type;
    uint32_t *offset;
    uint32_t size;
} __attribute__ ((packed)) DRVR_HEADER;

void *FindDriver(char *filename);
//tTask Prepare_Internal_Task(DRVR_HEADER *header, uint32_t file_start, uint32_t file_size, bool isv86);
//void Switch_Internal_Task(tTask *task, uint8_t priority);
char *driver_type(uint16_t type);
void run_v86_driver(uint32_t *file_start, uint32_t file_size, uint16_t flags);

//extern void v86_enter(uint32_t *task/*ss, uint32_t esp, uint32_t cs, uint32_t eip*/);

#endif