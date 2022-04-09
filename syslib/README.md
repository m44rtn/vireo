# Vireo System Library (syslib)
This repository contains the main application programming library for the Vireo kernel. The Vireo-II kernel is a mono-tasking kernel for the x86 CPU architecture. For more information, please refer to the [Vireo kernel](https://github.com/m44rtn/vireo-kernel) repository. To see examples of this library being used in practice, please refer to the [Vireo testing software](https://github.com/m44rtn/vireo-testing-software) repository,

## Building
Prerequisites: 
- `make`
- a cross-compiled `GCC (i686-elf)` and `binutils (i686-elf)` 
- `nasm`

Run `make` to compile a `.a` library which can be used to create programs for Vireo. 
