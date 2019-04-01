;v86.asm handles the assembly side of the virtual 8086 mode
;used by Vireo for the drivers

bits 32

%define     reg_ss  (esi)
%define     reg_esp (esi + 4)
%define     reg_cs  (esi + 8)
%define     reg_eip (esi + 12)

global v86_enter
extern Prep_TSS

v86_enter:
    mov esi, [esp + 4]
   
    .continue:
        mov ax, 0x23
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        call Prep_TSS

        push 0x23 ;push dword [esi]
        push dword [esi + 4]

        pushfd

        or dword [esp], 0x20202; 0x25202 ; (1 << 17) ;vm flag 0x20202 ; SHOULD BE OR'RED
       
        push 0x1B ;push dword [esi + 8]
        push dword [esi + 12]

        sti

        xor eax, eax
        xor ebx, ebx
        xor ecx, ecx
        xor edx, edx
        xor edi, edi
        xor esi, esi
        xor ebp, ebp ;could be dangerous
        
        iret


