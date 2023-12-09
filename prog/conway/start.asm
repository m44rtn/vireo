bits 32

global start
extern main

section .text
start:
call main ; launch program

ret
