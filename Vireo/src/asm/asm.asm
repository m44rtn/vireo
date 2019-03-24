
global PIT_initasm

PIT_initasm:
PITInit:
;256 ticks per second
 mov ax, 1193180 / 256

 mov al, 00110110b
 out 0x43, al

 out 0x40, al
 xchg ah, al
 out 0x40, al
ret




global PIT_readcount

extern counter

PIT_readcount:
pushfd
cli

mov al, 00000000b
out 0x43, al

in al, 0x40
mov ah, al
in al, 0x40
rol ax, 8

mov [counter], ax
popfd
ret





global enable_A20

enable_A20:
cli

call    a20wait
mov     al, 0xAD
out     0x64, al

call    a20wait
mov     al, 0xD0
out     0x64, al

call    a20wait2
in      al, 0x60
push    eax
 
call    a20wait
mov     al, 0xD1
out     0x64, al
 
call    a20wait
pop     eax
or      al, 2
out     0x60, al

call    a20wait
mov     al, 0xAE
out     0x64, al
 
call    a20wait
sti
ret
 
a20wait:
in      al, 0x64
test    al, 2
jnz     a20wait
ret
 
a20wait2:
in      al,0x64
test    al,1
jz      a20wait2
ret


global GetCPUVendor
extern cpuvend
extern VendorID

GetCPUVendor:
mov eax, 0x0
cpuid

mov [esi], ebx
shl esi, 4
mov [esi], edx
shl esi, 4
mov [esi], ecx

mov [VendorID], esi

ret

global TLBflush

TLBflush:
pushad
mov eax, [esp + 4]
invlpg [eax]
popad
ret

;global loadpdir
;extern pdir
;extern print

;loadpdir:
;pushad

;mov eax, [pdir]
;mov cr3, eax

;mov eax, cr0
;or eax, 0x80000001
;mov cr0, eax

;popad
;ret


global getESP
global StackPointer
getESP:
    mov eax, esp
    add eax, 4  ;compensate for return address?
    mov dword [StackPointer], eax 
    ret

global TSS_flush

TSS_flush:
mov ax, 0x2B
ltr ax
ret

global jmp_user_mode

jmp_user_mode:
    ;jumps to user mode -there's a lot that isn't right in this function
    ;input:
    ;   - a location of a function that should be executed by this thing
    ;output:
    ;   - n/a

mov ax, 0x23
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax

mov eax, [esp + 4]
push 0x23
push eax
pushf 

push 0x1B
iret

StackPointer dd 0
