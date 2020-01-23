bits 32

global PITInit

PITInit:
;1000 ticks per second?
 mov ax, 1193180 / 1000

 mov al, 00110110b
 out 0x43, al

 out 0x40, al
 xchg ah, al
 out 0x40, al
ret