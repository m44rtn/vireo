;MIT license
;Copyright (c) 2019 Maarten Vermeulen

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
extern cmain

section .multiboot
align 4
dd 0x1BADB002
dd 0x00
dd -(0x1BADB002 + 0x00)



section .text
start:
; does the initialization before we move on to the C part

; Set up the stack
mov esp, STACK_TOP

; push the location of the GRUB loader information onto the stack
push ebx

; TODO: GDT here, CPU init here, Paging init here

call cmain

HALT:
hlt
jmp HALT


;FIXME: all of these assembly functions (OUTB&INB) in seperate file?
global ASM_OUTB

ASM_OUTB:
; throws data to a port
;   input:
;       - port (first), WORD
;       - data (second), BYTE
;   output:
;       - N/A

push ebp
mov ebp, esp

; port is first, data second
mov edx, [ebp + 12]
mov eax, [ebp + 8]

out dx, al

mov esp, ebp
pop ebp

ret


global ASM_INB
ASM_INB:
; throws data to you when you ask it to
;   input:
;       - port, WORD
;   output:
;       - data, WORD

push ebp
mov ebp, esp

mov dx, [ebp + 8]

in al, dx

mov [ebp + 8], ax

mov esp, ebp
pop ebp

ret

section .bss
STACK_END:
    resb 0x4000
STACK_TOP:
