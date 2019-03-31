#include "screen.h"

//the defines
const uint8_t height = 25, width = 80, depth = 2;
int cursorX = 0, cursorY = 0;
int color = 0x07;

//char* vidmem = (char*) 0xb8000;


void updCurs(){
	
	unsigned temp;
	temp = cursorY * width + cursorX;
	
	//outportb asks for: (port to send to, the data to send)
	outb(0x3D4, 14);
	outb(0x3D5, temp >> 8);  // Send the high byte across the bus
    outb(0x3D4, 15);        // CRT Control Register: Select Send Low byte
    outb(0x3D5, temp); 
}





void setcolor(int clr){
	color = clr;
}

void trace(char* str, uint32_t val){
	uint16_t i = 0;  
	uint8_t length = strlen(str);

	for(i; i < length; i++){  //for i = less then the length of the char* do the following:
		switch(str[i]){
			case '%': {

			 switch(str[i+1]){
				 case 'i':
					print(hexstr(val));
					i++;
					break;
				case 's':
					print((char*) val);
					i++;
					break;
				default:
				putchar(str[i]);
			 }

			}
			break;
			default:
			putchar(str[i]);
			break;
		}
	}
}

void putchar (char c){
	char* vidmem = (char*) 0xb8000;
	int i = 0;
	switch(c){
			case ('\b'):
				cursorX--;
				vidmem[(cursorY * width + cursorX)*depth] = 0;	     //(0xF0 & color)  
			
			break;
			
			case ('\n'): //with '\n' do a new line
			cursorY++;
			cursorX = 0;
			break;
			
			case ('\t'):
			
			for(i; i < 4; i++){
				cursorX++;
			}
			break;

			case '\0':
				cursorY++;
				cursorX = 0;
			break;

			default:
			 vidmem[((cursorY * width + cursorX) * depth)] = c;
		     vidmem[((cursorY * width + cursorX)* depth + 1)] = color;
		     cursorX++;
			 break;
	}

	updCurs();
	linecheck();
	
}

/*Function: TextModeVideoMemDump()
 * Input: N/A
 * Output: Dump of video Memory
 */

char* TextModeVideoMemDump(){ 
	int x = 0;
	int y = 0;
	for(y; y < height; y++){
		for(x; x < width; x++){
			
		}
	}
}

/*Function: print()
 * Input: A message
 * Ouput: N/A
 */
 
void print(char* str/*, ...*/){
	
	uint16_t i = 0;  
	uint8_t length = strlen(str);

	for(i; i < length; i++){  //for i = less then the length of the char* do the following:
		putchar(str[i]);	
	}

}

void printline(char* str, int locX, int locY){
	
	uint16_t i = 0;  
	uint8_t length = strlen(str);
	int looping = 0;
	
	cursorX = locX;
	cursorY = locY;
	updCurs();

	for(i; i < length; i++){  //for i = less then the length of the char* do the following:
		putchar(str[i]);	
	}	
}

void linecheck(){
	if(cursorY >= height - 1){
		scroll(1);
	}
}

void clearl(uint8_t from, uint8_t to){
	
	uint16_t i = width * from * depth;
	char* vidmem = (char*) 0xb8000;
	
	for (i; i < (width*to*depth); i++){
		vidmem[(i / 2) * 2 + 1] = color;
		vidmem[(i / 2) * 2] = 0;
		//vidmem[i] = 0;
	}
}

void clearscr(){
	vidmem = (uint16_t *) 0xb8000;
	clearl(0, height);
	cursorX = 0;
	cursorY = 0;
	updCurs();
}


void scroll(uint8_t line){
	
	char* vidmemloc = (char*) 0xb8000; //location videomemory
	int i = 0;
	
	clearl(0, line - 1);

	for(i; i < width*(height-1)*depth; i++){
		//for(int y = 0; y < height; y++)
		//for(int x = 0; x < width; x++)
		vidmemloc[i] = vidmemloc[i + width*depth*line];
	}
	clearl(height-1-line,height-1);
	
	if((cursorY - line) < 0){   //did we clear much more then our screen is big?
		cursorY = 0;
		cursorX = 0;
	}
	else{
		cursorY -= line;
	}
	updCurs();
}

void disablecursor(){ //mainly for the Kernel Panic
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x3F);
}



void paintscr(int clr){
	uint16_t i = width * 0 * depth;
	char* vidmem = (char*) 0xb8000;
	color = clr;
	for (i; i < (width * height * depth); i++){
		vidmem[(i / 2) * 2 + 1] = clr;
		vidmem[(i / 2) * 2] = ' ';
		
	}
}



