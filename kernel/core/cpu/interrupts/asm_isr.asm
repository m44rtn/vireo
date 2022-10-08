bits 32

extern ASM_CPU_SAVE_STATE
extern state

global ISR_STANDARD
extern ISR_STANDARD_HANDLER
ISR_STANDARD:
pushad
    cld
    call ISR_STANDARD_HANDLER
popad
iret

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
extern ISR_01_HANDLER
ISR_01:
pushad
   push state
   call ASM_CPU_SAVE_STATE
   cld
   call ISR_01_HANDLER
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
; PIT
pushad
    cld
    call ISR_20_HANDLER  
popad
iret

global ISR_80
extern api_dispatcher
ISR_80:
; System calls

; save old eip
pop DWORD [ignore]
push DWORD [ignore]

pushad

push eax
push DWORD [ignore]

sti
call api_dispatcher

; manually pop the things we pushed on the stack for
; api_dispatcher
pop DWORD [ignore]
pop DWORD [ignore]

popad
iretd

; for ignoring values without using the registers
ignore dd 0x0
