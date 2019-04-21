;Will 'steal' the other's eax values and add them together

bits 32
jmp setup

setup:
    xor eax, eax
    jmp add_vals

add_vals:
    mov edi, [insert loc here]
    mov eax, DWORD [edi]

    mov edi, [insert loc here]
    mov ebx, DWORD [edi]

    add eax, ebx

    jmp add_vals