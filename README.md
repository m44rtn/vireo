# Vireo Kernel II

The Vireo kernel is an x86 kernel written in C.

## Goals of Vireo II
The goals of Vireo II are basically to learn something about operating systems and to make it easier for me to use older ports such as Serial and Parallel. Why support these older ports? Well, for example I've got an old
IBM 5160 over here and it may be useful to transfer files to it over Serial or Parallel. Maybe it has even more use in the future, who knows?

## Building
The Vireo II kernel is built using a cross compiler (i686-elf) from GCC version 8.3.0, using the C89 standard. You may be able to use a newer cross compiler, however it hasn't been tested. 
Just use `all` to build, `iso` to build an iso with grub and `run` to run the kernel in virtualbox 6.0+ (or `run-old` to run the kernel in an older virtualbox). To build you'll need a cross-compiler and for the iso you (may) need xorriso.

