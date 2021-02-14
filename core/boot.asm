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

global start
extern main

section .text
align 4
dd 0x1BADB002
dd 0x00
dd -(0x1BADB002 + 0x00)



;section .text
start:
; does the initialization before we move on to the C part
cli

mov DWORD [MAGICNUMBER], eax
mov DWORD [BOOTLOADER_STRUCT_ADDR], ebx

; Set up the stack
mov esp, STACK_TOP
mov ebp, STACK_END


; push the location of the GRUB loader information onto the stack
push ebx

call main

HALT:
hlt
jmp HALT

global MAGICNUMBER
MAGICNUMBER             dd 0 ; will have 0x2BADB002 in here if it is multiboot compliant

global BOOTLOADER_STRUCT_ADDR
BOOTLOADER_STRUCT_ADDR  dd 0


section .bss
global STACK_TOP
STACK_END:
    resb 0x4000
STACK_TOP:
