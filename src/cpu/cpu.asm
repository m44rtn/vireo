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