GCCPARAMS = -m32 -ffreestanding -o ../bin/obj/
ASPARAMS = -f elf32 -o ../bin/obj/
LDPARAMS = -m elf_i386

SETUPLOC = ../SETUP/


OBJFILES =  ../bin/obj/kasm.o  ../bin/obj/kc.o  ../bin/obj/sys.o  ../bin/obj/screen.o  ../bin/obj/gdt.o  ../bin/obj/gdtasm.o \
	 ../bin/obj/hardw.o  ../bin/obj/cpu.o  ../bin/obj/isr.o  ../bin/obj/util.o  ../bin/obj/idta.o  ../bin/obj/isra.o  ../bin/obj/asm.o  ../bin/obj/errorHandl.o  ../bin/obj/keyboard.o \
	 ../bin/obj/memory.o  ../bin/obj/pci.o  ../bin/obj/ahci.o  ../bin/obj/ATA.o  ../bin/obj/fat.o  ../bin/obj/hdd.o \
	 ../bin/obj/iso.o  ../bin/obj/DRVhandl.o  ../bin/obj/v86.o  ../bin/obj/multitasking.o  ../bin/obj/v86c.o  ../bin/obj/vesa.o

OBJDIR =  ../bin/obj	
#CFLAGS = -m32 -c -ffreestanding
#ASFLAGS = -f elf32





kernel: 
	nasm $(ASPARAMS)kasm.o kernel.asm
	nasm $(ASPARAMS)gdtasm.o  asm/gdt.asm
	nasm $(ASPARAMS)isra.o asm/isr.asm
	nasm $(ASPARAMS)idta.o asm/idt.asm
	nasm $(ASPARAMS)asm.o asm/asm.asm
	nasm $(ASPARAMS)stack.o asm/stack.asm
	nasm $(ASPARAMS)v86.o asm/v86.asm

	gcc $(GCCPARAMS)v86c.o -c  io/v86.c
	gcc $(GCCPARAMS)screen.o -c  drivers/screen/screen.c
	gcc $(GCCPARAMS)vesa.o -c  drivers/screen/VESA.c
	gcc $(GCCPARAMS)sys.o -c  io/sys.c
	gcc $(GCCPARAMS)gdt.o -c  io/gdt.c
	gcc $(GCCPARAMS)cpu.o -c  drivers/cpu.c
	gcc $(GCCPARAMS)isr.o -c  io/isr.c
	gcc $(GCCPARAMS)hardw.o -c  io/hardware.c
	gcc $(GCCPARAMS)util.o -c  io/util.c
	gcc $(GCCPARAMS)keyboard.o -c  drivers/keyboard.c
	gcc $(GCCPARAMS)memory.o -c  io/memory.c
	gcc $(GCCPARAMS)errorHandl.o -c  include/error.c
	
	gcc $(GCCPARAMS)ahci.o -c  drivers/ATA/SATA.c
	gcc $(GCCPARAMS)ATA.o -c  drivers/ATA/ATA.c
	gcc $(GCCPARAMS)hdd.o -c  drivers/HDD.c
	
	gcc $(GCCPARAMS)fat.o -c  drivers/FS/FAT.c
	gcc $(GCCPARAMS)iso.o -c  drivers/FS/ISO9660.c
	
	gcc $(GCCPARAMS)pci.o -c  io/PCI.c

	gcc $(GCCPARAMS)DRVhandl.o -c  drivers/DriverHandler.c
	gcc $(GCCPARAMS)multitasking.o -c  io/multitasking.c

	gcc $(GCCPARAMS)kc.o -c  kernel.c
		
	#STACK.O SHOULD ALWAYS BE THE LAST MODULE!
	ld $(LDPARAMS) -T linker.ld -o ../bin/kernel/kernel.sys $(OBJFILES)  ../bin/obj/stack.o -Map=kernel.map 

	rm ../grub/boot/kernel.sys
	cp -i ../bin/kernel/kernel.sys ../grub/boot/kernel.sys
	grub-mkrescue -o ../birdos.iso ../grub/

	#get xenops here: https://github.com/m44rtn/xenops
	xenops --file  include/version.h --build



module:
	nasm  modules/pVESA.asm -f bin -o modules/bin/VESA.SYS
	nasm  modules/Task1.asm -f bin -o modules/bin/TASK1.SYS
	nasm  modules/Task2.asm -f bin -o modules/bin/TASK2.SYS


run:
	#qemu-system-i386 -kernel bin/Build/kernel.sys
	virtualbox --startvm "Bird OS" --debug-command-line --start-running

setup:
	#nasm boot/loader.asm -o $(SETUPLOC)loader.sys
	mkisofs -o $(SETUPLOC)test_setup.iso modules/bin/VESA.SYS $(SETUPLOC)main.bat \
	$(SETUPLOC)install.bat modules/bin/TASK1.SYS modules/bin/TASK2.SYS