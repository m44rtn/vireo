;this will decrease eax

bits 32
nop
jmp setup

setup:
    mov eax, 0xFFFFFFFF
    jmp decrease

decrease:
    dec eax

    cmp eax, 0x10
    jle setup

    jmp decrease

