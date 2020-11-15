bits 32

extern ASM_CPU_SAVE_STATE
extern state

global ISR_00
extern ISR_00_HANDLER
ISR_00:
pushad
    push state
    call ASM_CPU_SAVE_STATE
    cld
    call ISR_00_HANDLER
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
extern ISR_05_HANDLER
ISR_05:
pushad
    push state
    call ASM_CPU_SAVE_STATE
    cld
    call ISR_05_HANDLER
    jmp $ 
popad
iret

global ISR_06
extern ISR_06_HANDLER
ISR_06:
pushad
    pushad
    push state
    call ASM_CPU_SAVE_STATE
    cld
    call ISR_06_HANDLER
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
extern ISR_0D_HANDLER
ISR_0D:
; general protection fault
pop DWORD [ignore]
pushad
    push state
    call ASM_CPU_SAVE_STATE
    cld
    call ISR_0D_HANDLER
    jmp $
popad
iret

global ISR_0E
extern ISR_0E_handler
ISR_0E:
; page fault
pop DWORD [ignore]
pushad
    push state
    call ASM_CPU_SAVE_STATE
    cld
    push DWORD [ignore]
    call ISR_0E_handler
    jmp $ 
popad
iret

global ISR_20
extern ISR_20_HANDLER
ISR_20:
pushad
    cld
    call ISR_20_HANDLER  
popad
iret

global ISR_21
extern ISR_21_HANDLER
ISR_21:
pushad
    cld
    call ISR_21_HANDLER 
popad
iret


; for ignoring values without tampering with the registers
ignore dd 0x0
