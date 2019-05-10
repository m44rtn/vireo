bits 32
global idt_set

idt_set:
mov eax, [esp + 256]
lidt [eax]
