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


uint8_t *screen;

void vesa_init(int x, int y, int d)
{
    tREGISTERS *registers = (tREGISTERS *) 0x4000;
	
    uint16_t mode = vesa_findmode(x, y, d);
    current_vid_mode_inf = (tMODE_INFO *) 0x3000;

    registers->ecx = mode;
	registers->eax = 0x4f01;
	registers->esi = 0x00;
	registers->edi = (uint32_t) current_vid_mode_inf;
	registers->es = 0;

	v86_interrupt(0x10, registers);

	kmemset((void *) registers, 0x000, sizeof(tREGISTERS));
	registers->eax = 0x4f02;
	registers->ebx = mode;
	registers->es = 0;
	registers->edi = (uint32_t) current_vid_mode_inf;

	v86_interrupt(0x10, registers);

    screen = (uint8_t *) current_vid_mode_inf->physbase;
}

uint16_t vesa_findmode(int x, int y, int d)
{
    tVBE_INFO *ctrl = (tVBE_INFO *) 0x2000;
    tMODE_INFO *info = (tMODE_INFO *) 0x3000;
    tREGISTERS *registers = (tREGISTERS *) 0x4000;

    uint16_t *modes;
    uint16_t best = 0x13;
    int pixdiff, bestpixdiff = vesa_difference(320 * 200, x * y);
    int depthdiff, bestdepthdiff = (8 >= d)? 8 - d : (d - 8) * 2;

    kmemset((void *) ctrl, 0, 512);
    ctrl->vbesign[0] = 'V';
    ctrl->vbesign[1] = 'B';
    ctrl->vbesign[2] = 'E';
    ctrl->vbesign[3] = '2';
   
    registers->eax = 0x4f00;
    registers->edi = (uint32_t) ctrl;
    registers->es = (uint32_t) 0;

    v86_interrupt(0x10, registers);

    if(registers->eax != 0x004F) return best;

    trace("ctrl = %i\n", (uint32_t) ctrl);
    
    modes = (uint16_t *) v86_sgoff_to_linear(ctrl->videoModeptr[1], ctrl->videoModeptr[0]);
    trace("modes = %i\n", v86_sgoff_to_linear(ctrl->videoModeptr[1], ctrl->videoModeptr[0]));

    for(uint32_t i = 0; modes[i] < 0xFFFF; i++)
    {
        kmemset((void *) info, 0, sizeof(tREGISTERS));
        registers->eax = 0x4f01;
        registers->ecx = (uint32_t) modes[i];
        registers->edi = (uint32_t) 0x3000;
        registers->es = 0;

        v86_interrupt(0x10, registers);

        if(registers->eax != 0x004F)  continue;

        if(!(info->attributes & 0x90)) continue;
        
        if(info->memory_model != 4 && info->memory_model != 6) continue;

        if(info->Xres == x && info->Yres == y && info->bpp == d) return modes[i] | 0x4000;

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

    best |= 0x4000;
    return best;
}


uint32_t vesa_difference(uint32_t low, uint32_t high)
{
    return high - low;
}

void vesa_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{

    //this only works for 800x600, needs to be 'rewritten'
    uint32_t where = (x * (current_vid_mode_inf->bpp / 8)) + (y * current_vid_mode_inf->bPerLine);
    screen[where] = color & 255;
    screen[where + 1] = (color >> 8) & 255;
    screen[where + 2] = (color >> 16) & 255;
}