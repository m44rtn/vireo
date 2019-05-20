#ifndef VESA_H
#define VESA_H

#include "../../include/types.h"
#include "../../io/v86.h"
#include "../../io/multitasking.h"
#include "../../io/memory.h"

typedef struct
{
    uint16_t attributes;
    uint8_t winA, winB;
    uint16_t granularity;
    uint16_t winsize;
    uint16_t WinsegA, WinsegB;
    uint32_t WinFuncptr;
    uint16_t bPerLine;

    uint16_t Xres, Yres;
    uint8_t Wchar, Ychar, planes, bpp, banks;
    uint8_t memory_model, bank_size, image_pages;
    uint8_t resv0;

    uint8_t red_mask, red_pos;
    uint8_t green_mask, green_pos;
    uint8_t blue_mask, blue_pos;
    uint8_t resv_mask, resv_pos;
    uint8_t directcolor_attrib;

    uint32_t physbase;
    uint32_t resv1;
    uint16_t resv2;
} __attribute__ ((packed)) tMODE_INFO;

tMODE_INFO *current_vid_mode_inf;

void vesa_init(int x, int y, int d);
uint16_t vesa_findmode(int x, int y, int d);
uint32_t vesa_difference(uint32_t low, uint32_t high);

void vesa_put_pixel(uint32_t x, uint32_t y, uint32_t color);
#endif