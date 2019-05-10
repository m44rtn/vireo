//Handles the 'monitoring' of the virtual 8086 environment


#include "v86.h"

///

uint32_t v86_linear_to_sgoff(uint32_t ptr)
{
    
    uint16_t segment, offset;
    //offset + 16*segment

    offset = ptr & 0xf;
    segment = (ptr - offset) / 16;
    return ((uint32_t) segment << 16 | offset);
}

/* not necessarry, so maybe just use this as logic
 * or is it?*/
uint32_t v86_sgoff_to_linear(uint16_t segment, uint16_t offset)
{
    uint32_t LinearAddress = (segment * 16) + offset;
    return LinearAddress;
}

void v86_interrupt(uint16_t interrupt, char *registers, ...)
{
    /* the location of the other arguments */
    uint32_t *other_stuff = (&interrupt +  sizeof(char*)) ;

    trace("other stuff %i", other_stuff[1]);
}


