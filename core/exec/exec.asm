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

; c header used for the functions in this file is exec/exec.h

section .text
global EXEC_CALL_FUNC

EXEC_CALL_FUNC:
;   calls with a pointer
;   input:
;       - function pointer
;       - data struct pointer
;   output:
;       - N/A

pushad

push ebp
mov ebp, esp

push DWORD [ebp + 32 + 12]
call DWORD [ebp + 32 + 8]

mov esp, ebp
pop ebp

popad

ret

.funcptr dd 0x00
.datstct dd 0x00


global asm_exec_call
section .text

asm_exec_call:
; calls an external binary
;	input: 
;       - pointer to binary    [stack]
;       - pointer to new stack [stack + 4]
;	ouput: n/a

push ebp
push esp

mov ebp, esp

; function to call
mov eax, DWORD [ebp + 12]
mov edi, eax

; new stack
mov eax, DWORD [ebp + 16]
mov esp, eax

call edi

pop esp
pop ebp

ret

global asm_exec_isr
section .text

asm_exec_isr:
; calls an external binary
;	input: 
;       - pointer to binary    [stack]
;	ouput: n/a

push ebp

mov ebp, esp

; function to call
mov eax, DWORD [ebp + 8]
mov edi, eax

call edi

pop esp
pop ebp

ret