;Supports fat16 or fat32, not sure
;(c) 2018

Bits	16
jmp	Main

convert_sector:
push	bx
push	ax
mov	bx, ax
mov	dx, 0
div	WORD [SectorsPerTrack]
add	dl, 01h
mov	cl, dl
mov	ax, bx
mov	dx, 0
div	WORD [SectorsPerTrack]
mov	dx, 0
div	WORD [Sides]
mov	dh, dl
mov	ch, al
pop	ax
pop	bx
mov	dl, BYTE [BootDrive]
ret

SectorsPerTrack	dw	18
Sides	dw	2

reset_floppy:
mov	ax, 0
mov	dl, BYTE [BootDrive]
int	13h
ret

;In=  si = char*, ah = 0eh, al = char, Out= charcater screen
Print:
lodsb
cmp	al, 0
je	Done
mov	ah, 0eh
int	10h
jmp	Print

Done:
ret

Main:
;setup stack
cli
mov	ax, 0x0000
mov	ss, ax
mov	sp, 0xFFFF
sti

;setup registers
mov	ax, 07C0h
mov	ds, ax
mov	es, ax

mov	[BootDrive], dl

mov	ch, 0
mov	cl, 2
mov	dh, 1
mov	bx, buffer
mov	al, 14
mov	ah, 2
pusha
load_root:
int	13h
jnc	loaded_root
call	reset_floppy
jmp	load_root

loaded_root:
popa
;cmpsb es:di with ds:si
mov	di, buffer
mov	cx, 224
search_root:
push	cx
pop	dx
mov	si, filename
mov	cx, 11
rep	cmpsb
je	found_file
add	ax, 32
mov	di, buffer
add	di, ax
push	dx
pop	cx

loop	search_root

xor	ah,ah
int	0x16			; wait for a key
int	0x18			; reboot the machine

msg	db	"Couldn't load system, press any key to reboot...",0

found_file:
mov	ax, WORD [di+15]
mov	[FirstSector], ax

mov	bx, buffer
mov	ax, 1
call	convert_sector
mov	al, 9
mov	ah, 2
pusha
load_fat:
int	13h
jnc	loaded_fat
call	reset_floppy
jmp	load_fat

loaded_fat:
mov	ah, 2
mov	al, 1
push	ax

load_file_sector:
mov	ax, WORD [FirstSector]
add	ax, 31
call	convert_sector
mov	ax, 2000h
mov	es, ax
mov	bx, WORD [Pointer]

pop	ax
push	ax

int	13h
jnc	calculate_next_sector
call	reset_floppy
jmp	load_file_sector

calculate_next_sector:
mov	ax, [FirstSector]
mov	dx, 0
mov	bx, 6
mul	bx
mov	bx, 4
div	bx
mov	si, buffer
add	si, ax
mov	ax, WORD [si]

or	dx, dx
jz	even

odd:
shr	ax, 4
jmp	short next_sector_calculated

even:
and	ax, 0FFFh

next_sector_calculated:
mov	WORD [FirstSector], ax
cmp	ax, 0FF8h
jae	end
add	WORD [Pointer], 512
jmp	load_file_sector

end:
pop	ax
mov	dl, BYTE [BootDrive]
jmp	2000h:0000h

BootDrive	db	0
filename	db	"STAGE2  BIN"
FirstSector	dw	0
Pointer	dw	0

times 510 - ($-$$)	db	0

dw	0xAA55

buffer:
