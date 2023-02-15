bits 32

global start
extern main

section .text
start:
push ecx
push edx
call main ; launch program

pop ecx
pop ecx

ret
