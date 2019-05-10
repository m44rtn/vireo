#ifndef SCREEN_H
#define SCREEN_H

#include "../../io/sys.h"
#include "../../include/types.h"
#include "../../include/info.h"
#include "../../io/util.h"

int cursorX, cursorY;
const uint8_t height, width, depth;
static uint16_t* vidmem;
static int cursor;


void updCurs();

void trace(char* str, uint32_t val);

void print(char* str/*, ...*/);
void printline(char* str, int locX, int locY);
void linecheck();
void scroll(uint8_t line);

void clearl(uint8_t from, uint8_t to);
void clearscr();

void disablecursor();
void putchar (char c);
void paintscr(int clr); //Painting the entire screen a specific color
void setcolor(int clr); //Setting the color for a specific piece of text

#endif
