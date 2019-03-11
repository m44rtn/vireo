;v86.asm handles the assembly side of the virtual 8086 mode
;used by Vireo for the drivers

global v86_enter
extern print

v86_enter:
    ;push ebp
    ;mov ebp, esp

    push .msg
    call print
    .continue:
        push dword [ebp + 8] ;ss
        push dword [ebp + 12] ;esp

        pushfd

        or dword [esp], (1 << 17) ;vm flag
        push dword [ebp + 12] ;cs
        push dword [ebp + 16] ;eip
        
        
        ;mov esp, ebp
        ;pop ebp
        iret

.msg db "Hello, world!\n", 0x00

v86_disable:
push ebp
mov ebp, esp

;disable the v86 mode
pushfd
and dword [esp], 0xFFFDFFFF
popfd

mov esp, ebp
pop ebp
ret


