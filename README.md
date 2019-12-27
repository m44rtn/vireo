# Vireo Kernel II

NOTE:TODO:FIXME: The REFMAN is not up-to-date

The Vireo kernel is an x86 kernel written in C.

This is the rewrite of the Vireo I kernel (see branch master). Vireo I was very
unstable and badly written abd didn't use a cross compiler or 'dynamic' makefile (in the sense that it searches for what to build on its own). Time for a rewrite! :)

The Wiki is currently not up-to-date.

## Goals of Vireo II
The goals of Vireo II are basically to learn something about operating systems and to make it easier for me to use older ports such as Serial and Parallel. Why support these older ports? Well, for example I've got an old
IBM 5160 over here and it may be useful to transfer files to it over Serial or Parallel. Maybe it has even more use in the future, who knows?

## Building
The Vireo II kernel is built using a cross compiler (i686-elf) from GCC version 8.3.0, using the C89 standard. You may be able to use a newer cross compiler, however it hasn't been tested. 
Just use `all` to build, `iso` to build an iso with grub and `run` to run the kernel in virtualbox. To build you'll need a cross-compiler and for the iso you (may) need xorriso.

