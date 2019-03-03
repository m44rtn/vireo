align 4				
dd 0x1BADB002		;magic number GRUB
dd 0x00
dd - (0x1BADB002+0x00)



bits 32

extern main	    
extern enable_A20	
global start
global paging	

section .text

start:
cli

;reserve a stack above the 1MiB mark
mov ebp, 0x10000000
mov esp, 0x100FFFFF

;call paging
push cs
push ss
push ebx ;for the GRUB loader's mem stuff

kernel:
call main		;Go to the C kernel
jmp $

paging:
call setup_dir
call setup_tbl
call setup_pag
ret


setup_dir:
    ;Sets up the paging directory to empty

    mov edx, 0
    mov ecx, 1024

    .setup_loop:
        mov eax, 0x0002
        mov DWORD [0xB00000 + edx*4], eax
        add edx, 1
    loop .setup_loop
    
ret

setup_tbl:
    ;sets up the paging directory

    mov edx, 0
    mov ecx, 1024
    .setup_loop:
        push ecx
        push edx

        cmp edx, 0xc0
        je .add_edx
      
        mov eax, edx
        mov ebx, 0x1000
        mul ebx
        or eax, 3

        pop edx
        push edx
        mov DWORD [0xC00000 + edx*4], eax

        .add_edx:
        pop edx
        add edx, 1
        pop ecx
    loop .setup_loop
ret

setup_pag:
    mov eax, 0xC00000
    or eax, 3
    mov DWORD [0xB00000 + edx*4], eax

    
    mov eax, 0xB00000
    mov cr3, eax
    ;jmp $
    
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    jmp $
    mov eax, 0
    mov ebp, 0

ret

Page_table:
    times 1024 dd 0x00

Page_Dir:
    times 1024 dd 0x00
