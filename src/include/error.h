#ifndef ERROR_H
#define ERROR_H

#include "types.h"
#include "../io/hardware.h"
#include "../drivers/screen/screen.h"
#include "../io/multitasking.h"

void kernel_panic(char* error);
void kernel_panic_dump(char* error);
void InitFail(char* fail);
void console_Warning(char* warning);

void error(int err);
#endif