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
global do_regs
global do_exor
global v86_ret

extern vesa_hello
extern Prep_TSS

extern trace
v86_enter:
    mov esi, [esp + 4]
    mov edi, [esp + 8]
    ;pusha
    
    ;mov eax, cr4
    ;or eax, 0x01
    ;mov cr4, eax

    pusha
    mov [edi + 16], esp

    .continue:
        ;mov ax, 0x23
        ;mov ds, ax       
        ;mov fs, ax
        ;mov gs, ax

        ;mov ax, 0x00
        ;mov es, ax
        

        call Prep_TSS

        push dword [esi] 
        push dword [esi + 4]

        pushfd

        or dword [esp], 0x20202

        push dword [esi + 8]
        push dword [esi + 12]

        push edi
        ;call do_exor
        pop edi
        call do_regs
        

        iret
.return:
popa
ret 


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
    mov eax, dword [edi]
    mov ecx, dword [edi + 4]
    mov edx, dword [edi + 8]
    mov ebx, dword [edi + 12]
    mov esi, dword [edi + 24]
    mov edi,  dword [edi + 28]
    
ret


v86_ret:
mov edi, 0x4000
mov eax, dword [edi + 16]
add eax, 8
mov esp, eax

push ss
push esp
pushfd
push cs
push v86_enter.return;dword [esp]

iretd
.string$ db "eax=%i", 0x00
