global gdt_set32
global CRzero
extern PagingSetup

gdt_set32:

mov eax, [esp + 4] ;get the pointer from the C part
lgdt [eax] 		   ;pass it to the CPU
mov ax, 0x10 	   ;load data segment registers
mov	ds, ax
mov	es, ax
mov	fs, ax
mov	gs, ax
mov	ss, ax

;call PagingSetup
;pop eax
;mov CR3, eax

mov eax, cr0
or eax, 00000001b ;0x80000001 ;00000001b
mov cr0, eax

jmp 0x08:.done
.done:
mov [CRzero], eax
ret


CRzero dd 0



