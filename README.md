# Vireo Kernel II

The Vireo kernel is an x86 kernel written in C.

A few facts and a warning:
- This project exists since 2014, and has had several rewrites now (hopefully this is the last)
- The project was originally called Bird OS
- It's written in C89. You may ask why, but I wouldn't have an answer for you
- I started with this rewrite (Vireo-II) in 2019
- If you think this code is terrible, don't look at the *VireoI* branch
- The code of Vireo-II *is* terrible

## License
The kernel is MIT licensed.

## Goals of Vireo-II
One of the goals of Vireo-II is to support older ports such as serial and parallel. Another goal is to have a flexible OS that can run on most older 32-bit systems (i686+) while also not being a memory hungry pacman.

## Building

### Tools needed
To build the Vireo-II kernel you will at least need GNU make, gcc-i686 (cross compiler), NASM, grub-pc and xorriso. Optional tools or programs are Virtualbox (for testing), [Xenops](https://github.com/m44rtn/xenops) (version keeping) and GNU ld-i686 (cross, for debugging using a .map file).

### Makefile commands
- To build the kernel and create a .iso file simply run `make` with no arguments
- Run `make all` to only build the kernel binary (./bin/kernel.sys)
- Run `make iso` to only create an iso image using an already built kernel binary (birdos.iso)
- Run `make run` or `make run-old` to launch a Virtualbox VM named "BirdOS" (preexisting) with debugging enabled
- Run `make clean` to remove all core/*.o files
- Run `make map` to create a .map file for debugging purposes (kernel.map)
- Run `make todo` to show a list of comments in the entire code base containing the words 'TODO' or 'FIXME'

### Configuring a Virtualbox VM
In order for the `make run` command to work, you will need to have a few things configured beforehand. First thing is to create a VM named "BirdOS". You can also use a different name, in which case you will need to change the `VM_NAME` variable in the makefile.

After this, you will need to make sure the VM boots from CD and make sure the birdos.iso file is attached to the IDE controller of the VM. You can also do this while the VM has already been launched by going to **Devices > Optical Drives**.

## Documentation
> There was an attempt...

We've got this README, an occassional comment here and there and a very outdated 'refman'. 