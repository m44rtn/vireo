/*
MIT license
Copyright (c) 2022 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __SCANCODE_H__
#define __SCANCODE_H__

#include "types.h"

#define KEYCODE_FLAG_KEY_RELEASED 0x1000

#define KEYCODE_UNUSED      0x0000
#define KEYCODE_ESC         0x0001
#define KEYCODE_F1          0x0002
#define KEYCODE_F2          0x0003
#define KEYCODE_F3          0x0004
#define KEYCODE_F4          0x0005
#define KEYCODE_F5          0x0006
#define KEYCODE_F6          0x0007
#define KEYCODE_F7          0x0008
#define KEYCODE_F8          0x0009
#define KEYCODE_F9          0x000a
#define KEYCODE_F10         0x000b
#define KEYCODE_F11         0x000c
#define KEYCODE_F12         0x000d

#define KEYCODE_BACKTICK    0x0101
#define KEYCODE_1           0x0102
#define KEYCODE_2           0x0103
#define KEYCODE_3           0x0104
#define KEYCODE_4           0x0105
#define KEYCODE_5           0x0106
#define KEYCODE_6           0x0107
#define KEYCODE_7           0x0108
#define KEYCODE_8           0x0109
#define KEYCODE_9           0x010a
#define KEYCODE_0           0x010b
#define KEYCODE_MINUS       0x010c
#define KEYCODE_EQUALS      0x010d
#define KEYCODE_BACKSPACE   0x010e

#define KEYCODE_TAB         0x0201
#define KEYCODE_Q           0x0202
#define KEYCODE_W           0x0203
#define KEYCODE_E           0x0204
#define KEYCODE_R           0x0205
#define KEYCODE_T           0x0206
#define KEYCODE_Y           0x0207
#define KEYCODE_U           0x0208
#define KEYCODE_I           0x0209
#define KEYCODE_O           0x020a
#define KEYCODE_P           0x020b
#define KEYCODE_SQBROPEN    0x020c // square bracket open
#define KEYCODE_SQBRCLOSE   0x020d // square bracket close
#define KEYCODE_BACKSLASH   0x020c

#define KEYCODE_CAPSLOCK    0x0301 
#define KEYCODE_A           0x0302
#define KEYCODE_S           0x0303
#define KEYCODE_D           0x0304
#define KEYCODE_F           0x0305
#define KEYCODE_G           0x0306
#define KEYCODE_H           0x0307
#define KEYCODE_J           0x0308
#define KEYCODE_K           0x0309
#define KEYCODE_L           0x030a
#define KEYCODE_SEMICOLON   0x030b
#define KEYCODE_QUOTE       0x030c
#define KEYCODE_ENTER       0x030d

#define KEYCODE_LSHIFT      0x0401
#define KEYCODE_Z           0x0402
#define KEYCODE_X           0x0403
#define KEYCODE_C           0x0404
#define KEYCODE_V           0x0405
#define KEYCODE_B           0x0406
#define KEYCODE_N           0x0407
#define KEYCODE_M           0x0408
#define KEYCODE_COMMA       0x0409
#define KEYCODE_PERIOD      0x040a
#define KEYCODE_FWDSLASH    0x040b
#define KEYCODE_RSHIFT      0x040c

#define KEYCODE_LCTRL       0x0501
#define KEYCODE_LWIN        0x0502
#define KEYCODE_LALT        0x0503
#define KEYCODE_SPACE       0x0504
#define KEYCODE_RALT        0x0505
#define KEYCODE_RWIN        0x0506
#define KEYCODE_MENU        0x0507
#define KEYCODE_RCTRL       0x0508

// here comes navigation (prtscreen, pageup and down, etc)
#define KEYCODE_PRINTSCR      0x0601
#define KEYCODE_PAUSEBREAK    0x0602
#define KEYCODE_SCROLLLOCK    0x0603
#define KEYCODE_INSERT        0x0604
#define KEYCODE_HOME          0x0605
#define KEYCODE_PAGEUP        0x0606
#define KEYCODE_DELETE        0x0607
#define KEYCODE_END           0x0608
#define KEYCODE_PAGEDOWN      0x0609
#define KEYCODE_UPCURSOR      0x060a
#define KEYCODE_LEFTCURSOR    0x060b
#define KEYCODE_DOWNCURSOR    0x060c
#define KEYCODE_RIGHTCURSOR   0x060d

#define KEYCODE_KEYP_0        0x0701
#define KEYCODE_KEYP_1        0x0702
#define KEYCODE_KEYP_2        0x0703
#define KEYCODE_KEYP_3        0x0704
#define KEYCODE_KEYP_4        0x0705
#define KEYCODE_KEYP_5        0x0706
#define KEYCODE_KEYP_6        0x0707
#define KEYCODE_KEYP_7        0x0708
#define KEYCODE_KEYP_8        0x0709
#define KEYCODE_KEYP_9        0x070a
#define KEYCODE_KEYP_PERIOD   0x070b
#define KEYCODE_KEYP_ENTER    0x070c
#define KEYCODE_KEYP_PLUS     0x070d
#define KEYCODE_KEYP_MINUS    0x070e
#define KEYCODE_KEYP_MULTIPLY 0x070f
#define KEYCODE_KEYP_SLASH    0x0710
#define KEYCODE_NUMLOCK       0x0711

#define KEYCODE_MM_WWWSEARCH  0x0801
#define KEYCODE_MM_PREVTRACK  0x0802
#define KEYCODE_MM_WWWFAVS    0x0803
#define KEYCODE_MM_WWWREFRESH 0x0804
#define KEYCODE_MM_VOLDOWN    0x0805
#define KEYCODE_MM_MUTE       0x0806
#define KEYCODE_MM_STOP       0x0807
#define KEYCODE_MM_CALCULATOR 0x0808
#define KEYCODE_MM_FORWARD    0x0809
#define KEYCODE_MM_VOLUP      0x080a
#define KEYCODE_MM_PLAYPAUSE  0x080b
#define KEYCODE_MM_WWWBACK    0x080c
#define KEYCODE_MM_WWWHOME    0x080d
#define KEYCODE_MM_STOP       0x080e
#define KEYCODE_MM_MYPC       0x080f
#define KEYCODE_MM_EMAIL      0x0810
#define KEYCODE_MM_NEXTTRACK  0x0811
#define KEYCODE_MM_MEDIASLCT  0x0812 // MEDIA SELECT

#define KEYCODE_ACPI_PWR      0x0901
#define KEYCODE_ACPI_SLEEP    0x0902
#define KEYCODE_ACPI_WAKE     0x0903

#endif // __SCANCODE_H__