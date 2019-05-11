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
} __attribute__ ((packed)) tVBE_INFO;

typedef struct
{
    /* data */
} tMODE_INFO;


uint16_t vesa_findmode(int x, int y, int d)
{
    tVBE_INFO *ctrl = (tVBE_INFO *) 0x2000;
    tMODE_INFO *info = (tMODE_INFO *) 0x3000;
    tREGISTERS *registers = (tREGISTERS *) 0x4000;

    uint16_t *modes;
    int i;
    uint16_t best = 0x13;
    int pixdiff, bestpixdiff = vesa_difference(320 * 200, x * y);
    int depthdiff, bestdepthdiff = (8 >= d)? 8 - d : (d - 8) * 2;

    registers->eax = 0x4f00;
    registers->edi = v86_linear_to_sgoff(0x2000) & 0xFFFF;
    registers->esi = (v86_linear_to_sgoff(0x2000) >> 16) & 0xFFFF;

    v86_interrupt(0x10, 0x4000);
    print("Hello, VESA world!\n");
    
    /* todo, oh and different way of doing v86 -> with calling a function to exec int? */
    //v86_interrupt();

}

void vesa_hello()
{
    print("Hello, VESA world!\n");
}

int vesa_difference(int low, int high)
{
    return high - low;
}