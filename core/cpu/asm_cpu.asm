;MIT license
;Copyright (c) 2020 Maarten Vermeulen

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


; CPU_SAVE_STATE: these are the defines for the struct (you can find it in the C code somewhere)
%define CPU_SAVE_STATE_EAX      (esi + 28)
%define CPU_SAVE_STATE_ECX      (esi + 24)
%define CPU_SAVE_STATE_EDX      (esi + 20)
%define CPU_SAVE_STATE_EBX      (esi + 16)
%define CPU_SAVE_STATE_ESP      (esi + 12)
%define CPU_SAVE_STATE_EBP      (esi + 08)
%define CPU_SAVE_STATE_ESI      (esi + 04)
%define CPU_SAVE_STATE_EDI      (esi + 00)

; CPU SAVE STATE: these are the defines for the stack, it looks nicer
%define CPU_SAVE_STATE_STACK_EAX      (ebp + 40)
%define CPU_SAVE_STATE_STACK_ECX      (ebp + 36)
%define CPU_SAVE_STATE_STACK_EDX      (ebp + 32)
%define CPU_SAVE_STATE_STACK_EBX      (ebp + 28)
%define CPU_SAVE_STATE_STACK_ESP      (ebp + 24)
%define CPU_SAVE_STATE_STACK_EBP      (ebp + 20)
%define CPU_SAVE_STATE_STACK_ESI      (ebp + 16)
%define CPU_SAVE_STATE_STACK_EDI      (ebp + 12)

bits 32

;for lack of a better name
global ASM_GDT_SUBMIT
ASM_GDT_SUBMIT:
; gives the location of the GDT descriptor to the cpu
;   input:
;       - pointer to the GDT descriptor
;   output:
;       - N/A

    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    lgdt [eax]

    mov eax, cr0
    or eax, 0001b
    mov cr0, eax

    ;should be 0x08
    jmp 0x08:.SEGMENT_RELOAD

    .SEGMENT_RELOAD:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax   

    mov esp, ebp
    pop ebp
ret

global ASM_IDT_SUBMIT
ASM_IDT_SUBMIT:
; gives the IDT descriptor location to the cpu
;   input:
;       - pointer to the IDT
;   output:
;       - N/A

    push ebp
    mov ebp, esp

    mov edx, [ebp + 8]
    lidt [edx]
    sti

    mov esp, ebp
    pop ebp
ret

global ASM_CPU_PAGING_ENABLE
ASM_CPU_PAGING_ENABLE:
; hopefully this enables paging correctly
;   input:
;       - pointer to the page table
;   output:
;       - N/A

    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov cr3, eax
   
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    mov esp, ebp
    pop ebp
ret

global ASM_CPU_INVLPG
ASM_CPU_INVLPG:
; invalidates a page
; input:
;   - physical address where the page table points to
; output
;   - N/A
    push ebp
    mov ebp, esp

    mov eax, ebp
    add eax, 8

    invlpg [eax]

    mov esp, ebp
    pop ebp
ret


global ASM_CPU_SAVE_STATE
ASM_CPU_SAVE_STATE:
; saves the current state of the cpu. it uses 
; pushad (so that has to be done already before calling this function)
; input:
;   - pushad
;   - pointer to struct to store the values
; output:
;   - N/A
    push ebp
    mov ebp, esp

    mov esi, [ebp + 8]

    mov eax, [CPU_SAVE_STATE_STACK_EDI]
    mov [CPU_SAVE_STATE_EDI], eax

    mov eax, [CPU_SAVE_STATE_STACK_ESI]
    mov [CPU_SAVE_STATE_ESI], eax

    mov eax, [CPU_SAVE_STATE_STACK_EBP]
    mov [CPU_SAVE_STATE_EBP], eax

    mov eax, [CPU_SAVE_STATE_STACK_ESP]
    mov [CPU_SAVE_STATE_ESP], eax

    mov eax, [CPU_SAVE_STATE_STACK_EBX]
    mov [CPU_SAVE_STATE_EBX], eax
    
    mov eax, [CPU_SAVE_STATE_STACK_EDX]
    mov [CPU_SAVE_STATE_EDX], eax

    mov eax, [CPU_SAVE_STATE_STACK_ECX]
    mov [CPU_SAVE_STATE_ECX], eax

    mov eax, [CPU_SAVE_STATE_STACK_EAX]
    mov [CPU_SAVE_STATE_EAX], eax

    ; TODO: eip

    mov esp, ebp
    pop ebp
ret


global ASM_CHECK_CPUID
ASM_CHECK_CPUID:
; check if CPUID is supported
;   input:
;       - N/A
;   output:
;       - 1 if supported, 0 if unsupported (in CPUID_AVAILABLE)

    pushfd
     
    mov eax, DWORD [esp]   

    popfd   

    and eax, 0x200000
    cmp eax, 0x200000
    jne .done

    mov BYTE [CPUID_AVAILABLE], 1

    .done:
ret

global ASM_CPU_GETVENDOR
ASM_CPU_GETVENDOR:
; gets the vendor id of the cpu
;   input:
;       - N/A
;   output:
;       - Vendor string (in var CPUID_VENDOR_STRING)

    mov eax, 0
    cpuid

    mov DWORD [CPUID_SUPPORTED_FUNCTIONS], eax

    mov DWORD [CPUID_VENDOR + 0x00], ebx
    mov DWORD [CPUID_VENDOR + 0x04], edx
    mov DWORD [CPUID_VENDOR + 0x08], ecx 

    mov eax, CPUID_VENDOR  
    mov DWORD [CPUID_VENDOR_STRING], eax 

ret


global ASM_CPU_GETNAME
ASM_CPU_GETNAME:
; gets the cpu name string of the cpu
;   input:
;       - N/A
;   output:
;       - Vendor string (in var CPUID_CPUNAME_STRING)

    mov eax, 0x80000002
    cpuid

    mov DWORD [CPUID_CPUNAME + 0x00], eax
    mov DWORD [CPUID_CPUNAME + 0x04], ebx
    mov DWORD [CPUID_CPUNAME + 0x08], ecx
    mov DWORD [CPUID_CPUNAME + 0x0C], edx  

    mov eax, 0x80000003
    cpuid

    mov DWORD [CPUID_CPUNAME + 0x10], eax
    mov DWORD [CPUID_CPUNAME + 0x14], ebx
    mov DWORD [CPUID_CPUNAME + 0x18], ecx
    mov DWORD [CPUID_CPUNAME + 0x1C], edx  

    mov eax, 0x80000004
    cpuid

    mov DWORD [CPUID_CPUNAME + 0x20], eax
    mov DWORD [CPUID_CPUNAME + 0x24], ebx
    mov DWORD [CPUID_CPUNAME + 0x28], ecx
    mov DWORD [CPUID_CPUNAME + 0x2C], edx  

    mov eax, CPUID_CPUNAME
    mov DWORD [CPUID_CPUNAME_STRING], eax

ret

; for the cpuid functions
global CPUID_AVAILABLE
CPUID_AVAILABLE db 0

global CPUID_VENDOR_STRING
CPUID_VENDOR_STRING dd 0
CPUID_VENDOR times 13 db 0

global CPUID_CPUNAME_STRING
CPUID_CPUNAME_STRING dd 0
CPUID_CPUNAME times 48 db 0

global CPUID_SUPPORTED_FUNCTIONS
CPUID_SUPPORTED_FUNCTIONS dd 0