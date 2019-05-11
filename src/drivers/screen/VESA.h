#ifndef VESA_H
#define VESA_H

#include "../../include/types.h"
#include "../../io/v86.h"
#include "../../io/multitasking.h"
#include "../../io/memory.h"

uint16_t vesa_findmode(int x, int y, int d);
uint32_t vesa_difference(uint32_t low, uint32_t high);

void vesa_put_pixel(uint32_t x, uint32_t y, uint32_t color);
#endif