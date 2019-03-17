;v86.asm handles the assembly side of the virtual 8086 mode
;used by Vireo for the drivers

bits 32

%define     reg_ss  (esi)
%define     reg_esp (esi + 4)
%define     reg_cs  (esi + 8)
%define     reg_eip (esi + 12)

global v86_enter
extern print

v86_enter:
    
    mov esi, [esp + 4]
    ;pop esi
    .continue:

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

;mov eax, [esp + 4]

;push 0x23
;push eax
;pushf 

;push 0x1B
;iret
        push 0x23 ;push dword [esi]
        push dword [esi + 4]

        pushfd

        or dword [esp], 0x20202 ; (1 << 17) ;vm flag 0x20202 ; SHOULD BE OR'RED

        push 0x1B ;push dword [esi + 8]
        push dword [esi + 12]

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor edi, edi
    xor esi, esi
        
        iret

.msg db "Hello, world!\n", 0x00

;v86_disable:
;push ebp
;mov ebp, esp

;disable the v86 mode
;pushfd
;and dword [esp], 0xFFFDFFFF
;popfd

;mov esp, ebp
;pop ebp
;ret


