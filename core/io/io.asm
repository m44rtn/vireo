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

section .text
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
mov eax, [ebp + 12]
mov edx, [ebp + 8]

out dx, al

mov esp, ebp
pop ebp

ret

global ASM_OUTL
ASM_OUTL:
; throws data to a port
;   input:
;       - port (first), WORD
;       - data (second), DWORD
;   output:
;       - N/A

push ebp
mov ebp, esp

; data
mov eax, [ebp + 12]
; port
mov edx, [ebp + 8] 

out dx, eax

mov esp, ebp
pop ebp

ret


global ASM_INB
ASM_INB:
; throws data to you from a port when you ask it to
;   input:
;       - port, WORD
;   output:
;       - data, WORD

push ebp
mov ebp, esp

mov dx, [ebp + 8]

xor eax, eax

in al, dx

mov [ebp + 8], ax

mov esp, ebp
pop ebp

ret

global ASM_INL
ASM_INL:
; throws data to you from a port when you ask it to
;   input:
;       - port, WORD
;   output:
;       - data, DWORD

push ebp
mov ebp, esp

mov dx, [ebp + 8]

xor eax, eax

in eax, dx

mov [ebp + 8], eax

mov esp, ebp
pop ebp

ret

global ASM_INSW
ASM_INSW:
; gets as much data from a port as you would like
;   input:
;       - port, WORD
;       - amount of words, DWORD
;       - buffer
;   output:
;       - N/A

push ebp
mov ebp, esp

mov edi, DWORD [ebp + 16] ; buffer
mov ecx, DWORD [ebp + 12] ; words
mov edx, DWORD [ebp + 8]  ; port

rep insw

mov esp, ebp
pop ebp

ret


global ASM_IOWAIT
ASM_IOWAIT:
; uses the 'unused' port 0x80 to wait for IO OP's to complete
;   input:
;       - N/A
;   output:
;       - N/A

push ebp
mov ebp, esp

mov al, 0
out 0x80, al

mov esp, ebp
pop ebp

ret