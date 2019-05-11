#ifndef VESA_H
#define VESA_H

#include "../../include/types.h"
#include "../v86.h"
#include "../../io/multitasking.h"

uint16_t vesa_findmode(int x, int y, int d);
void vesa_hello();
#endif