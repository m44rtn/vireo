;this will increment eax

bits 32
jmp setup

setup:
    xor eax, eax
    jmp increment

increment:
    inc eax

    cmp eax, 0xFFFFFF10
    jge setup

    jmp increment