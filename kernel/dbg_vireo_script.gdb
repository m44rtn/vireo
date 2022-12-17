add-symbol-file bin/vireo.sys 0x0000000000100000
target remote | qemu-system-i386 -S -gdb stdio -cdrom ../vireo.iso -m 4
