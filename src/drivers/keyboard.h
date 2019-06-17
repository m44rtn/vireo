#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../include/types.h"
#include "../include/info.h"
#include "../include/error.h"
#include "../include/keych.h"
#include "../io/hardware.h"
#include "../io/util.h"

#include "../io/memory.h"
#include "../drivers/screen/screen.h"

void ps2_keyb_init();
void ps2_mouse_init();

void mouse_write(uint8_t byte);
uint8_t mouse_read();
void mouse_wait(uint8_t type);

void ps2_wait_write();
void ps2_wait_read();

void keyboard_clear_buffers();
void hang_for_key(char key);
bool IS_pressed(char key);
void keyboard_putonscr(char key);
void keybin(char key);

bool DRIVE_ERROR_DETECT(uint16_t *buf, uint32_t BufSize_WORDS);

#endif
