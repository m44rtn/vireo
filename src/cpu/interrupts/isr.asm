bits 32

global ISR_00
extern ISR_00_HANDLER
ISR_00:
pusha
    cld
    call ISR_00_HANDLER
    jmp $ 
popa
iret

global ISR_01
ISR_01:
pusha
    jmp $ 
popa
iret

global ISR_02
ISR_02:
pusha
    jmp $ 
popa
iret

global ISR_03
ISR_03:
pusha
    jmp $ 
popa
iret

global ISR_04
ISR_04:
pusha
    jmp $ 
popa
iret

global ISR_05
ISR_05:
pusha
    jmp $ 
popa
iret

global ISR_06
ISR_06:
pusha
    jmp $ 
popa
iret

global ISR_07
ISR_07:
pusha
    jmp $ 
popa
iret

global ISR_08
ISR_08:
pusha
    jmp $ 
popa
iret

global ISR_09
ISR_09:
pusha
    jmp $ 
popa
iret

global ISR_0A
ISR_0A:
pusha
    jmp $ 
popa
iret

global ISR_0B
ISR_0B:
pusha
    jmp $ 
popa
iret

global ISR_0C
ISR_0C:
pusha
    jmp $ 
popa
iret

global ISR_0D
ISR_0D:
pusha
    jmp $ 
popa
iret

global ISR_0E
ISR_0E:
pusha
    jmp $ 
popa
iret

global ISR_20
ISR_20:
pusha
    jmp $ 
popa
iret

global ISR_21
ISR_21:
pusha
    jmp $ 
popa
iret


