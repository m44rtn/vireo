bits 32

global ISR_00
extern ISR_00_HANDLER
ISR_00:
pushad
    jmp ISR_00_HANDLER
    jmp $ 
popad
iret

global ISR_01
ISR_01:
pushad
    jmp $ 
popad
iret

global ISR_02
ISR_02:
pushad
    jmp $ 
popad
iret

global ISR_03
ISR_03:
pushad
    jmp $ 
popad
iret

global ISR_04
ISR_04:
pushad
    jmp $ 
popad
iret

global ISR_05
ISR_05:
pushad
    jmp $ 
popad
iret

global ISR_06
ISR_06:
pushad
    jmp $ 
popad
iret

global ISR_07
ISR_07:
pushad
    jmp $ 
popad
iret

global ISR_08
ISR_08:
pushad
    jmp $ 
popad
iret

global ISR_09
ISR_09:
pushad
    jmp $ 
popad
iret

global ISR_0A
ISR_0A:
pushad
    jmp $ 
popad
iret

global ISR_0B
ISR_0B:
pushad
    jmp $ 
popad
iret

global ISR_0C
ISR_0C:
pushad
    jmp $ 
popad
iret

global ISR_0D
ISR_0D:
pushad
    jmp $ 
popad
iret

global ISR_0E
ISR_0E:
pushad
    jmp $ 
popad
iret

global ISR_20
ISR_20:
pushad
    jmp $ 
popad
iret

global ISR_21
ISR_21:
pushad
    jmp $ 
popad
iret


