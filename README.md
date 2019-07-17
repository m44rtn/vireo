# Vireo Kernel II

The Vireo kernel is an x86 kernel written in C.

This is the rewrite of the Vireo I kernel (see branch master). Vireo I was very
unstable and badly written. Moreover, it didn't use a cross compiler or dynamic makefile. These issues are fixed in this version.

The Wiki is currently not up-to-date.

## Goals of Vireo II
The goal of Vireo II is to improve on Vireo I, and also add lots of features
which didn't make it into the first kernel. This includes but is not limited to:
- Multitasking
- ELF file loading and executing
- FAT32 write support

It should also be written so that it doesn't expect anything (e.g. that there's a harddrive installed). Apart from this, it should be made a simpler kernel to code in and run.

In contrast to Vireo I, Vireo II will be more thought out. 

Eventually, GRUB may be replaced by a custom bootloader. The custom bootloader is far from being done, so that will probably take quite a while.

## Building
The Vireo II kernel is built using a cross compiler (i686-elf) from GCC version 8.3.0, using the C89 standard. You may be able to use a newer cross compiler, however it hasn't been tested. 

Extra things you need:
- xorriso (for now)
- Xenops, for the kernel version
- Virtualbox, for running the kernel

Just run `make all` and you SHOULD be fine. If you get any linker errors, try `make clean` first. 
