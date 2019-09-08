bits 32

global ASM_GDT_SUBMIT
ASM_GDT_SUBMIT:
; gives the GDT descriptor to the cpu
;   input:
;       - descriptor
;   output:
;       - N/A

    mov eax, [esp + 4]
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
ret
