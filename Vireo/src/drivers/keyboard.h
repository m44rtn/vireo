#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../include/types.h"
#include "../include/info.h"
#include "../include/error.h"
#include "../io/hardware.h"
#include "../io/util.h"

#include "../io/memory.h"
#include "../drivers/screen/screen.h"

void hang_for_key(char key);
bool IS_pressed(char key);
void putonscr(char chr);
void keybin(char key);

bool DRIVE_ERROR_DETECT(uint16_t *buf, uint32_t BufSize_WORDS);

#endif
