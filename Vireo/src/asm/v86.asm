;v86.asm handles the assembly side of the virtual 8086 mode
;used by Vireo for the drivers

bits 32

%define     reg_ss  (esi)
%define     reg_esp (esi + 4)
%define     reg_cs  (esi + 8)
%define     reg_eip (esi + 12)
%define     command (esi + 16)
%define     reg_edi (esi + 20)

global v86_enter
extern Prep_TSS

extern trace
v86_enter:
    mov esi, [esp + 4]
    ;mov edi, [esp + 8]

    push dword [edi + 4]
    push .string$
    call trace
   
    .continue:
        mov ax, 0x23
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        call Prep_TSS

        
        
        push dword [esi] ;0x23
        push dword [esi + 4]

        pushfd

        or dword [esp], 0x20202; 0x25202 ; (1 << 17) ;vm flag 0x20202 ; SHOULD BE OR'RED

        push dword [esi + 8] ;0x1B
        push dword [esi + 12]

        sti
        call do_regs

        iret
        .string$ db "ecx=%i", 0x00

do_exor:
    xor eax, eax
    xor ecx, ecx
    xor edx, edx
    xor ebx, ebx
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
ret


do_regs:
    ;mov eax, 
    push dword [edi]
    ;mov ecx, 
    push dword [edi + 4]
    ;mov edx, 
    push dword [edi + 8]
    ;mov ebx, 
    push dword [edi + 12]
    ;mov esp, dword [edi + 16] ;we could delete this
    ;mov ebp, dword [edi + 20]

    ;mov esi, 
    push dword [edi + 24]
    ;mov edi, 
    push dword [edi + 28]
    popad
ret
