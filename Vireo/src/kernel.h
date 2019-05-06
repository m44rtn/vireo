#ifndef KERNEL_H
#define KERNEL_H




void main(multiboot_info_t* mbh,  uint32_t ss, uint32_t cs);
void setsysinf();
void ver(char* verinf);

void main_set_flags(uint32_t flags);

extern void halt();

int GRUBMAGIC = 0x1BADB002;

#endif
