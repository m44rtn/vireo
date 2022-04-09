;MIT license
;Copyright (c) 2019-2021 Maarten Vermeulen

;Permission is hereby granted, free of charge, to any person obtaining a copy
;of this software and associated documentation files (the "Software"), to deal
;in the Software without restriction, including without limitation the rights
;to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;copies of the Software, and to permit persons to whom the Software is
;furnished to do so, subject to the following conditions:
;
;The above copyright notice and this permission notice shall be included in all
;copies or substantial portions of the Software.
;
;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;SOFTWARE.

bits 32

%define FREQUENCY 1000

global PITInit
PITInit:
; here we calculate the reload value needed for the wanted frequency
mov eax, 3579545 
mov ebx, FREQUENCY
mov edx, 0
div ebx

mov ebx, 3
mov edx, 0
div ebx

mov ebx, eax

 mov al, 00110110b
 out 0x43, al

 
 mov eax, ebx
 out 0x40, al
 rol ax, 8
 out 0x40, al

 rol ax, 8
ret