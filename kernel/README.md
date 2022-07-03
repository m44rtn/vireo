# Vireo Kernel II

The Vireo kernel is an x86 kernel written in C. It will be a mono-tasking kernel that should still be very flexible.

Currently, this is work in progress and the quality of code can be dissapointing.

## License
The kernel is MIT licensed.

## Building

### Tools needed
To build the Vireo-II kernel you will at least need GNU make, gcc-i686 (cross compiler), NASM, grub-pc and xorriso. Optional tools or programs are Virtualbox (for testing), [Xenops](https://github.com/m44rtn/xenops) (version keeping) and GNU ld-i686 (cross, for debugging using a .map file).

### Makefile commands
- To build the kernel and create a .iso file simply run `make` with no arguments
- Run `make all` to only build the kernel binary (./bin/vireo.sys)
- Run `make iso` to only create an iso image using an already built kernel binary (../vireo.iso)
- Run `make run` or `make run-old` to launch a Virtualbox VM named "Vireo" (preexisting) with debugging enabled
- Run `make clean` to remove all core/*.o files
- Run `make map` to create a .map file for debugging purposes (kernel.map)
- Run `make todo` to show a list of comments in the entire code base containing the words 'TODO' or 'FIXME'

### Configuring a Virtualbox VM
In order for the `make run` command to work, you will need to have a few things configured beforehand. First thing is to create a Virtualbox virtual machine (VM) named "Vireo". You can also use a different name, in which case you will need to change the `VM_NAME` variable in the makefile.

After this, you will need to make sure the VM boots from CD and make sure the vireo.iso file is attached to the IDE controller of the VM. You can also do this while the VM has already been launched by going to **Devices > Optical Drives** and hitting [Right ctrl + R] to reboot the VM.

## Documentation
Currently, the documentation provided leaves something to be desired. However, this will change in the future.