/*
MIT license
Copyright (c) 2019 Maarten Vermeulen

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

#include "screen_basic.h"

#include "../basic/io.h"
#include "../include/types.h"


#define SCREEN_BASIC_WIDTH          80
#define SCREEN_BASIC_HEIGHT         25
#define SCREEN_BASIC_DEPTH          2

#define SCREEN_BASIC_DEFAULT_COLOR  0x07

typedef struct SCREENDATA
{
    unsigned short cursorY;
    unsigned short cursorX;
    unsigned char chScreenColor;
} SCREENDATA;

static SCREENDATA SCRscreenData;

/* Static function defines */
static void screen_basic_update_cursor(void);
static void screen_basic_char_put_on_screen(char c);
static void screen_basic_scroll(unsigned char line);
static void screen_basic_linecheck(void);
static void screen_basic_clear_line(unsigned char from, unsigned char to);

/* TODO: code comment */

/*
 * 'Public' part
 */

void screen_basic_init(void)
{
    screen_basic_set_screen_color(SCREEN_BASIC_DEFAULT_COLOR);
    SCRscreenData.cursorY = SCRscreenData.cursorX = 0;
    
    screen_basic_clear_screen();
}

void screen_basic_set_screen_color(unsigned char color)
{
    SCRscreenData.chScreenColor = color;
}
    
void trace(char* str, unsigned int val)
{
	/*uint16_t i = 0;  
	uint8_t length = strlen(str);

	for(i; i < length; i++){ 
		switch(str[i]){
			case '%': {

			 switch(str[i+1]){
				 case 'i':
					screen_basic_char_put_on_screen(hexstr(val));
					i++;
					break;
				case 's':
					screen_basic_char_put_on_screen((char*) val);
					i++;
					break;
				default:
				screen_basic_char_put_on_screen(str[i]);
			 }

			}
			break;
			default:
			screen_basic_char_put_on_screen(str[i]);
			break;
		}
	}*/
}

void print(char* str){
	
	uint16_t i = 0;  
	uint8_t length = 15; /* strlen(str);*/

	for(i; i < length; i++){  //for i = less then the length of the char* do the following:
		screen_basic_char_put_on_screen(str[i]);	
	}

}

void screen_basic_clear_screen(void){
	uint16_t *vidmem = (uint16_t *) 0xb8000;
	screen_basic_clear_line(0, SCREEN_BASIC_HEIGHT);
	SCRscreenData.cursorX = 0;
	SCRscreenData.cursorY = 0;
	screen_basic_update_cursor();
}

/*
 * 'Private' part
 */
static void screen_basic_update_cursor(void)
{
    unsigned temp;

    temp = SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX;
	
	ASM_OUTB(0x3D4, 14);
	ASM_OUTB(0x3D5, temp >> 8);
    ASM_OUTB(0x3D4, 15);
    ASM_OUTB(0x3D5, temp);   
}

static void screen_basic_char_put_on_screen(char c){
	char* vidmem = (char*) 0xb8000;
	int i = 0;
	switch(c){
			case ('\b'):
				SCRscreenData.cursorX--;
				vidmem[(SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX)*SCREEN_BASIC_DEPTH] = 0;  
			
			break;
			
			case ('\n'):
			SCRscreenData.cursorY++;
			SCRscreenData.cursorX = 0;
			break;
			
			case ('\t'):
			
			for(i; i < 4; i++){
				SCRscreenData.cursorX++;
			}
			break;

			case '\0':
				SCRscreenData.cursorY++;
				SCRscreenData.cursorX = 0;
			break;

			default:
			 vidmem[((SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX) * SCREEN_BASIC_DEPTH)] = c;
		     vidmem[((SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX)* SCREEN_BASIC_DEPTH + 1)] = SCRscreenData.chScreenColor;
		     SCRscreenData.cursorX++;
			 break;
	}

	screen_basic_update_cursor();
	screen_basic_linecheck();	
}

static void screen_basic_linecheck(void)
{
	if(SCRscreenData.cursorY >= SCREEN_BASIC_HEIGHT - 1){
		screen_basic_scroll(1);
	}
}

static void screen_basic_scroll(unsigned char line)
{
	
	char* vidmemloc = (char*) 0xb8000;
	int i = 0;
	
	screen_basic_clear_line(0, line - 1);

	for(i; i < SCREEN_BASIC_WIDTH * (SCREEN_BASIC_HEIGHT - 1) * SCREEN_BASIC_DEPTH; i++){
		vidmemloc[i] = vidmemloc[i + SCREEN_BASIC_WIDTH*SCREEN_BASIC_DEPTH*line];
	}
	screen_basic_clear_line(SCREEN_BASIC_HEIGHT-1-line,SCREEN_BASIC_HEIGHT-1);
	
	if((SCRscreenData.cursorY - line) < 0){
		SCRscreenData.cursorY = 0;
		SCRscreenData.cursorX = 0;
	}
	else{
		SCRscreenData.cursorY -= line;
	}
	screen_basic_update_cursor();
}

static void screen_basic_clear_line(unsigned char from, unsigned char to)
{
	
	unsigned short i = SCREEN_BASIC_WIDTH * from * SCREEN_BASIC_DEPTH;
	char* vidmem = (char*) 0xb8000;
	
	for (i; i < (SCREEN_BASIC_WIDTH*to*SCREEN_BASIC_DEPTH); i++){
		vidmem[(i / 2) * 2 + 1] = SCRscreenData.chScreenColor;
		vidmem[(i / 2) * 2] = 0;
		/*vidmem[i] = 0;*/
	}
}

