/* * */

#include "VESA.h"

typedef struct
{
    /* data */
} tVBE_INFO;

typedef struct
{
    /* data */
} tMODE_INFO;


uint16_t vesa_findmode(int x, int y, int d)
{
    tVBE_INFO *ctrl = (tVBE_INFO *) 0x2000;
    tMODE_INFO *info = (tMODE_INFO *) 0x3000;

    uint16_t *modes;
    int i;
    uint16_t best = 0x13;
    int pixdiff, bestpixdiff = vesa_difference(320 * 200, x * y);
    int depthdiff, bestdepthdiff = (8 >= d)? 8 - d : (d - 8) * 2;

    /* todo, oh and different way of doing v86 -> with calling a function to exec int? */
    //v86_interrupt();

}

int vesa_difference(int low, int high)
{
    return high - low;
}