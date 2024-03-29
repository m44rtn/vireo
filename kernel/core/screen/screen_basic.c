/*
MIT license
Copyright (c) 2019-2023 Maarten Vermeulen

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

#include "../api/api.h"
#include "../api/syscalls.h"

#include "../io/io.h"
#include "../include/types.h"
#include "../include/exit_code.h"

#include "../util/util.h"

#include "../memory/paging.h"

#define SCREEN_BASIC_DEFAULT_COLOR  0x07

#define SCREEN_BASIC_INDENT_AMOUNT	8

#define SCREEN_BASIC_FIRST_SCNLINE	0
#define SCREEN_BASIC_LAST_SCNLINE	15

#define SCREEN_BASIC_MARGIN_BELL	7 // characters, named after the margin bell on typewriters

#define VGA_VIDEO_BUFFER			0xb8000

typedef struct SCREENDATA
{
	unsigned short cursorY;
	unsigned short cursorX;
    uint32_t SCREEN_FLAGS;

	unsigned char chScreenColor;
} SCREENDATA;

typedef struct api_screen_t
{
    syscall_hdr_t hdr;
    const char *str;
    uint32_t x;
    uint32_t y;
} __attribute__((packed)) api_screen_t;

typedef struct screen_color_t
{
    syscall_hdr_t hdr;
    uint8_t color;
} __attribute__((packed)) api_screen_color_t;

typedef enum screen_mode_t
{
    VGA_MODE_3      // aka BIOS/'default' text mode
} screen_mode_t;

typedef struct screen_info_t
{
    screen_mode_t mode;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} __attribute__((packed)) screen_info_t;

static SCREENDATA SCRscreenData;
uint8_t hexdigits = 0;

/* Static function defines */
static void screen_basic_char_put_on_screen(char c);
static void screen_basic_move_cursor_internal(void);
static void screen_basic_scroll(unsigned char line);
static void screen_basic_linecheck(char c);
static void screen_basic_clear_line(unsigned char from, unsigned char to);

/*
 * 'Public' part
 */
void screen_basic_api(void *req)
{
	api_screen_t *r = (api_screen_t *) req;

	switch(r->hdr.system_call)
	{
		default:
			r->hdr.exit_code = EXIT_CODE_GLOBAL_NOT_IMPLEMENTED;
		break;

		case SYSCALL_GET_SCREEN_INFO:
		{
			screen_info_t *scr = (screen_info_t *) evalloc(sizeof(screen_info_t), prog_get_current_running());
			
			scr->mode = VGA_MODE_3;
			scr->depth = SCREEN_BASIC_DEPTH;
			scr->width = SCREEN_BASIC_WIDTH;
			scr->height = SCREEN_BASIC_HEIGHT;

			r->hdr.response_ptr = (void *) scr;
			r->hdr.response_size = sizeof(screen_info_t);
			break;
		}

		case SYSCALL_PRINT:
			print(r->str);
		break;

		case SYSCALL_PRINT_AT:
		{
			print_at(r->str, r->x, r->y);
			break;
		}

		case SYSCALL_GET_SCREEN_BUFFER:
			r->hdr.response_ptr = evalloc(SCREEN_BASIC_WIDTH * SCREEN_BASIC_HEIGHT * SCREEN_BASIC_DEPTH, prog_get_current_running());
			r->hdr.response_size = SCREEN_BASIC_WIDTH * SCREEN_BASIC_HEIGHT * SCREEN_BASIC_DEPTH;

			memcpy(r->hdr.response_ptr, (void *) VGA_VIDEO_BUFFER, r->hdr.response_size);
		break;

		case SYSCALL_GET_SCREEN_GET_BYTE:
			r->hdr.response= (uint32_t) screen_basic_getchar(r->x, r->y);
		break;

		case SYSCALL_SET_SCREEN_COLOR:
		{
			api_screen_color_t *c = (api_screen_color_t *) req;
			screen_basic_set_screen_color(c->color);
			break;
		}

		case SYSCALL_SET_SCREEN_CURSOR:
			if(r->x >= SCREEN_BASIC_WIDTH || r->y >= SCREEN_BASIC_HEIGHT)
				{r->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }
			
			SCRscreenData.cursorX = (uint16_t) r->x;
			SCRscreenData.cursorY = (uint16_t) r->y;

			r->hdr.exit_code = screen_basic_move_cursor((uint16_t) r->x, (uint16_t) r->y);
		break;

		case SYSCALL_GET_SCREEN_CURSOR:
			// You can get the value of the x and y cursor by doing the following calculation:
			// uint16_t x = pos % SCREEN_BASIC_WIDTH;
			// uint16_t y = pos / SCREEN_BASIC_WIDTH;

			r->hdr.response = (uint32_t) screen_basic_get_cursor_position();
		break;

		case SYSCALL_CLEAR_SCREEN:
			screen_basic_clear_screen();
		break;

		case SYSCALL_SCREEN_PUT_CHAR:
			if(r->x >= SCREEN_BASIC_WIDTH || r->y >= SCREEN_BASIC_HEIGHT)
				{r->hdr.exit_code = EXIT_CODE_GLOBAL_INVALID; break; }

			screen_basic_putchar(r->x, r->y, r->str[0]);
		break;
	}
}


unsigned char screen_basic_init(void)
{
	uint8_t check;

	SCRscreenData.SCREEN_FLAGS = 0;

    screen_basic_set_screen_color(SCREEN_BASIC_DEFAULT_COLOR);

    SCRscreenData.cursorY = 0;
	SCRscreenData.cursorX = 0;
    
    screen_basic_clear_screen();

	/* Enable the cursor and put it at the top */
	screen_basic_enable_cursor(SCREEN_BASIC_FIRST_SCNLINE, SCREEN_BASIC_LAST_SCNLINE);
	check = screen_basic_move_cursor(SCRscreenData.cursorY, SCRscreenData.cursorY);

	if(check == EXIT_CODE_GLOBAL_SUCCESS) SCRscreenData.SCREEN_FLAGS |= SCREEN_BASIC_CURSOR_ENABLED;

	return EXIT_CODE_GLOBAL_SUCCESS;
}

void screen_basic_enable_cursor(unsigned char cursor_start, unsigned char cursor_end)
{
	
	outb(0x3D4, 0x0A);
	outb(0x3D5, (uint8_t) ((inb(0x3D5U) & 0xC0U) | (cursor_start & 0x0FU)));

	outb(0x3D4, 0x0B);
	outb(0x3D5, (uint8_t) ((inb(0x3D5U) & 0xE0U) | (cursor_end & 0x0FU)));


}

void screen_basic_disable_cursor(void)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

uint8_t screen_basic_move_cursor(unsigned short x, unsigned short y)
{
	uint16_t verify_pos;
	uint16_t position = (uint16_t) (y * SCREEN_BASIC_WIDTH + x);

	/* just to be sure? */
	if(x >= SCREEN_BASIC_WIDTH) x = SCREEN_BASIC_WIDTH - 1;
	if(y >= SCREEN_BASIC_HEIGHT) y = SCREEN_BASIC_HEIGHT - 1;

	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((position >> 8) & 0xFF));

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (position & 0xFF));
	
	verify_pos = screen_basic_get_cursor_position();

	if(position != verify_pos) return SCREEN_BASIC_EXIT_CODE_CURSOR_MOVE_FAIL;
	
	return EXIT_CODE_GLOBAL_SUCCESS;
}

unsigned short screen_basic_get_cursor_position(void)
{
	uint16_t position = 0;

	outb(0x3D4, 0x0F);
	position |= (uint16_t) inb(0x3D5);

	outb(0x3D4, 0x0E);
	position |= (uint16_t) (inb(0x3D5) <<  8);

	return position;
}

void screen_basic_set_screen_color(unsigned char color)
{
    SCRscreenData.chScreenColor = color;
}

void screen_set_hexdigits(uint8_t value)
{
	hexdigits = value;
}

void print_value(const char* str, unsigned int val)
{
	unsigned int i;  
	unsigned int length = strlen((char *) str);

	for(i = 0; i < length; i++){ 
		switch(str[i]){
			case '%': {

			 switch(str[i+1]){
				 case 'x':
					print(hexstr(val, hexdigits));
					i++;
					break;
				 case 'i':
					print(intstr(val));
					i++;
					break;
				case 's':
					print((char*) val);
					i++;
					break;
				case 'c':
					screen_basic_char_put_on_screen((char) val);
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
	}
}

void print(const char* str){
	
	unsigned int i;  
	unsigned int length = strlen((char *) str);

	for(i = 0; i < length; i++){ 
		screen_basic_char_put_on_screen(str[i]);	
	}

}

void print_at(const char *str, uint32_t x, uint32_t y)
{
	uint16_t _x = SCRscreenData.cursorX;
	uint16_t _y = SCRscreenData.cursorY;

	SCRscreenData.cursorX = (uint16_t) x;
	SCRscreenData.cursorY = (uint16_t) y;

	print(str);

	SCRscreenData.cursorX = _x; 
	SCRscreenData.cursorY = _y;

}

void screen_basic_clear_screen(void)
{
	screen_basic_clear_line(0, SCREEN_BASIC_HEIGHT);
	SCRscreenData.cursorX = 0;
	SCRscreenData.cursorY = 0;
	screen_basic_move_cursor(SCRscreenData.cursorX, SCRscreenData.cursorY);
}


char screen_basic_getchar(unsigned int x, unsigned int y)
{
	unsigned char* vidmem = (unsigned char*) VGA_VIDEO_BUFFER;

	if(x > SCREEN_BASIC_WIDTH || y > SCREEN_BASIC_HEIGHT)
		return NULL;

	return (char) (vidmem[((y * SCREEN_BASIC_WIDTH + x) * SCREEN_BASIC_DEPTH)]);
}

void screen_basic_putchar(unsigned int x, unsigned int y, char c)
{
	unsigned char* vidmem = (unsigned char*) VGA_VIDEO_BUFFER;

	if(x > SCREEN_BASIC_WIDTH || y > SCREEN_BASIC_HEIGHT)
		return;

	vidmem[((y * SCREEN_BASIC_WIDTH + x) * SCREEN_BASIC_DEPTH)] = (unsigned char) c;
	vidmem[((y * SCREEN_BASIC_WIDTH + x)* SCREEN_BASIC_DEPTH + 1)] = SCRscreenData.chScreenColor;
}

/*
 * 'Private' part
 */

static void screen_basic_char_put_on_screen(char c){
	unsigned char* vidmem = (unsigned char*) VGA_VIDEO_BUFFER;

	switch(c){
			case ('\b'):

				if(SCRscreenData.cursorX == 0 && SCRscreenData.cursorY)
					{ SCRscreenData.cursorX = SCREEN_BASIC_WIDTH; SCRscreenData.cursorY--; }

				if(SCRscreenData.cursorX != 0)
					SCRscreenData.cursorX--;
					
				vidmem[(SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX)*SCREEN_BASIC_DEPTH] = 0;
			break;
			
			case ('\n'):
			SCRscreenData.cursorY++;
			SCRscreenData.cursorX = 0;
			break;
			
			case ('\t'):
			{
				uint16_t a = SCRscreenData.cursorX / SCREEN_BASIC_INDENT_AMOUNT;
				SCRscreenData.cursorX = (uint16_t) ((a + 1) * SCREEN_BASIC_INDENT_AMOUNT);
				break;
			}
			

			default:
			 vidmem[((SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX) * SCREEN_BASIC_DEPTH)] = (unsigned char) c;
		     vidmem[((SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX)* SCREEN_BASIC_DEPTH + 1)] = SCRscreenData.chScreenColor;
		     SCRscreenData.cursorX++;
			 break;
	}

	screen_basic_move_cursor_internal();
	screen_basic_linecheck(c);	
}

static void screen_basic_move_cursor_internal(void)
{
	uint16_t position = (uint16_t) (SCRscreenData.cursorY * SCREEN_BASIC_WIDTH + SCRscreenData.cursorX);

	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((position >> 8) & 0xFF));

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (position & 0xFF));
}

static void screen_basic_put_whitespace(uint32_t x_from, uint32_t x_to)
{
	for(; x_from < x_to; x_from++)
		screen_basic_putchar(x_from, SCRscreenData.cursorY, ' ');
}

static void screen_basic_linecheck(char c)
{

	if(SCRscreenData.cursorX >= SCREEN_BASIC_WIDTH || (c == ' ' && SCRscreenData.cursorX >= (SCREEN_BASIC_WIDTH - SCREEN_BASIC_MARGIN_BELL)))
	{
		// clear remaining space after margin bell, just in case something was left there.
		screen_basic_put_whitespace(SCRscreenData.cursorX, SCREEN_BASIC_WIDTH);

		SCRscreenData.cursorX = 0;
		SCRscreenData.cursorY++;
	}

	if(SCRscreenData.cursorY >= SCREEN_BASIC_HEIGHT - 1)
		screen_basic_scroll(1);

}

static void screen_basic_scroll(unsigned char line)
{
	
	char* vidmemloc = (char*) VGA_VIDEO_BUFFER;
	const unsigned short EndOfScreen = SCREEN_BASIC_WIDTH * (SCREEN_BASIC_HEIGHT - 1) * SCREEN_BASIC_DEPTH;
	unsigned short i;
	
	screen_basic_clear_line(0, (unsigned char) (line - 1));

	for(i = 0; i < EndOfScreen; i++){
		/* a signed integer will be fine for here */
		vidmemloc[i] = vidmemloc[(int) (i + SCREEN_BASIC_WIDTH * SCREEN_BASIC_DEPTH * line)];
	}
	screen_basic_clear_line( (unsigned char) (SCREEN_BASIC_HEIGHT -  1 - line), (SCREEN_BASIC_HEIGHT));
	
	if((SCRscreenData.cursorY - line) < 0){
		SCRscreenData.cursorY = 0;
		SCRscreenData.cursorX = 0;
	}
	else{
		SCRscreenData.cursorY = (unsigned short) (SCRscreenData.cursorY - line);
	}
	screen_basic_move_cursor(SCRscreenData.cursorX, SCRscreenData.cursorY);
}

static void screen_basic_clear_line(unsigned char from, unsigned char to)
{
	unsigned short i = (unsigned short) (SCREEN_BASIC_WIDTH * from * SCREEN_BASIC_DEPTH);
	char* vidmem = (char*) VGA_VIDEO_BUFFER;
	
	for (; i < (SCREEN_BASIC_WIDTH*to*SCREEN_BASIC_DEPTH); i++){
		vidmem[(i / 2) * 2 + 1] = (char) SCRscreenData.chScreenColor;
		vidmem[(i / 2) * 2] = 0;
		/*vidmem[i] = 0;*/
	}
}

