/* * */

#include "VESA.h"

typedef struct
{
    char vbesign[4];
    uint16_t version;
    uint16_t OEMstrPTR[2];
    uint8_t capabilities[4];
    uint16_t videoModeptr[2];
    uint16_t totalMem;
    uint16_t OEMsftwrRev;
    uint32_t OEMvendorNamePtr;
    uint32_t OEMproductNamePtr;
    uint32_t ProductRevPtr;
    uint8_t resv[222];
    uint8_t OEMdata[256];
} __attribute__ ((packed)) tVBE_INFO;

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
} tMODE_INFO;


uint8_t *screen = 0xdfff1dd6;// VGA: 0xA0000;

uint16_t vesa_findmode(int x, int y, int d)
{
    tVBE_INFO *ctrl = (tVBE_INFO *) 0x2000;
    tMODE_INFO *info = (tMODE_INFO *) 0x3000;
    tREGISTERS *registers = (tREGISTERS *) 0x4000;

    uint16_t *modes;
    uint16_t best = 0x13;
    int pixdiff, bestpixdiff = vesa_difference(320 * 200, x * y);
    int depthdiff, bestdepthdiff = (8 >= d)? 8 - d : (d - 8) * 2;

    kmemset(0x2000, 0, 512);
    ctrl->vbesign[0] = 'V';
    ctrl->vbesign[1] = 'B';
    ctrl->vbesign[2] = 'E';
    ctrl->vbesign[3] = '2';

    registers->eax = 0x4F00;
    registers->edi = 0x2000;

    v86_interrupt(0x10, registers);

    trace("registers->eax = %i\n", registers->eax);
    if(registers->eax != 0x004F) return best;
   
    trace("tVBE_INFO->signature = %s \n", ctrl->vbesign);
    trace("tVBE_INFO->version = %i \n", ctrl->version);
    
    modes = (uint16_t *) ctrl->videoModeptr;
    //trace("modes = %i\n", v86_sgoff_to_linear(ctrl->videoModeptr[1], ctrl->videoModeptr[0]));
    for(uint32_t i = 0; modes[i] < 0xFFFF; i++)
    {
        //kmemset(0x4000, 0, sizeof(tREGISTERS));
        registers->eax = 0x4f01;
        registers->ecx = (uint32_t) modes[i];
        registers->edi = 0x3000;

        v86_interrupt(0x10, 0x4000);

        if(registers->eax != 0x004F)  continue;

        if(!(info->attributes & 0x90)) continue;
        print("register == 0x004F");

        if(info->memory_model != 4 && info->memory_model != 6) continue;

        if(info->Xres == x && info->Yres == y && info->bpp == d) return modes[i];

        pixdiff = vesa_difference(info->Xres * info->Yres, x * y);
        depthdiff = (info->bpp >= d)? info->bpp - d : (d - info->bpp) * 2;
        if(bestpixdiff > pixdiff || bestpixdiff == pixdiff && bestdepthdiff > depthdiff)
        {
            best = modes[i];
            bestpixdiff = pixdiff;
            bestdepthdiff = depthdiff;
        }
    }
    if(x == 640 & y == 480 && d == 1) return 0x11;
    return best | 0x4000;
}


uint32_t vesa_difference(uint32_t low, uint32_t high)
{
    return high - low;
}

void vesa_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t where = x * 4 + y * 3200;
    screen[where] = color & 255;
    screen[where + 1] = (color >> 8) & 255;
    screen[where + 2] = (color >> 16) & 255;
}