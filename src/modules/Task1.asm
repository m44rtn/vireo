;this will decrease eax

bits 32
nop
jmp setup

setup:
    mov eax, 0x80000000
    jmp decrease

decrease:
    dec eax

    cmp eax, 0x10
    jle setup

    jmp decrease

