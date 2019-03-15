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
    ;push ebp
    ;mov eax, esp
    ;mov ebp, esp
    ;pusha
    
    mov esi, [esp + 4]
    ;pop esi
    .continue:
        push dword [esi]
        push dword [esi + 4]

        pushfd

        or dword [esp], 0x20202 ; (1 << 17) ;vm flag 0x20202 ; SHOULD BE OR'RED

        
        push dword [esi + 8]
        push dword [esi + 12]
        
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


