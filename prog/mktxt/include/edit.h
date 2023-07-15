#ifndef __EDIT_H__
#define __EDIT_H__

#include "types.h"

uint8_t edit_perform_input(uint16_t *keyb_bfr, uint32_t entries, char *path);
void edit_print(void);
err_t edit(api_space_t kb_api, char *path);


#endif
