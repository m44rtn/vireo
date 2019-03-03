bits 16
org 0x0600


;note to slef mimic fd loader


start:
cli
mov ax,0x0000
mov ss, ax
mov sp, 0xFFFF

mov bp, 0x7c00
sti

mov ax, 0x0600
mov ds, ax
mov es, ax

mov BYTE [bootdrive], 80h ;set to first harddrive

mov ah, 2
mov al, 14
mov ch, 0
mov cl, 2
mov dh, 1
mov dl, BYTE [bootdrive]
mov bx, buffr
pusha

load_root:
int 13h
jnc root_loaded

;reset drive
mov ah, 0
mov dl, BYTE [bootdrive]
int 13h
jmp load_root

root_loaded:
popa
mov di, buffr
mov cx, 224


bootdrive db 0
filename db "BIRDOS  SYS"

;It's not a bootloader sooooooo...
;times 510 - ($-$$) db 0
;dw 0xAA55


buffr:
