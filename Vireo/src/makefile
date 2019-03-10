GCCPARAMS = -m32 -ffreestanding
ASPARAMS = -f elf32
LDPARAMS = -m elf_i386

SETUPLOC = ../../SETUP/

#CFLAGS = -m32 -c -ffreestanding
#ASFLAGS = -f elf32


kernel: 
	gcc $(GCCPARAMS) -o bin/kc.o -c kernel.c
	nasm $(ASPARAMS) -o bin/kasm.o kernel.asm
	gcc $(GCCPARAMS) -o bin/screen.o -c drivers/screen.c
	gcc $(GCCPARAMS) -o bin/sys.o -c io/sys.c
	gcc $(GCCPARAMS) -o bin/gdt.o -c io/gdt.c
	nasm $(ASPARAMS) -o bin/gdtasm.o  asm/gdt.asm
	nasm $(ASPARAMS) -o bin/isra.o asm/isr.asm
	nasm $(ASPARAMS) -o bin/idta.o asm/idt.asm
	nasm $(ASPARAMS) -o bin/asm.o asm/asm.asm
	nasm $(ASPARAMS) -o bin/v86.o asm/v86.asm
	gcc $(GCCPARAMS) -o bin/cpu.o -c drivers/cpu.c
	gcc $(GCCPARAMS) -o bin/isr.o -c io/isr.c
	gcc $(GCCPARAMS) -o bin/hardw.o -c io/hardware.c
	gcc $(GCCPARAMS) -o bin/util.o -c io/util.c
	gcc $(GCCPARAMS) -o bin/keyboard.o -c drivers/keyboard.c
	gcc $(GCCPARAMS) -o bin/memory.o -c io/memory.c
	gcc $(GCCPARAMS) -o bin/errorHandl.o -c include/error.c
	
	gcc $(GCCPARAMS) -o bin/ahci.o -c drivers/ATA/SATA.c
	gcc $(GCCPARAMS) -o bin/ATA.o -c drivers/ATA/ATA.c
	gcc $(GCCPARAMS) -o bin/hdd.o -c drivers/HDD.c
	#nasm $(ASPARAMS) -o bin/atasm.o storage/ATA.asm
	gcc $(GCCPARAMS) -o bin/mfat.o -c drivers/FS/MFAT.c
	gcc $(GCCPARAMS) -o bin/fat.o -c drivers/FS/FAT.c
	gcc $(GCCPARAMS) -o bin/iso.o -c drivers/FS/ISO9660.c
	
	gcc $(GCCPARAMS) -o bin/pci.o -c io/PCI.c

	gcc $(GCCPARAMS) -o bin/DRVhandl.o -c drivers/DriverHandler.c
	gcc $(GCCPARAMS) -o bin/multitasking.o -c io/multitasking.c
		
	#nasm -f bin -o bin/Build/loader.sys loader.asm
	ld $(LDPARAMS) -T linker.ld -o bin/Build/kernel.sys bin/kasm.o bin/kc.o bin/sys.o bin/screen.o bin/gdt.o bin/gdtasm.o \
	bin/hardw.o bin/cpu.o bin/isr.o bin/util.o bin/idta.o bin/isra.o bin/asm.o bin/errorHandl.o bin/keyboard.o \
	bin/memory.o bin/pci.o bin/ahci.o bin/ATA.o bin/mfat.o bin/fat.o bin/hdd.o bin/iso.o bin/DRVhandl.o \
	bin/v86.o bin/multitasking.o
	#ld $(LDPARAMS) -T linker.ld -o bin/Build/kernel.bin bin/kasm.o bin/kc.o bin/sys.o bin/screen.o bin/gdt.o bin/gdtasm.o bin/hardw.o bin/cpu.o bin/isr.o bin/util.o bin/reshut.o bin/idta.o bin/isra.o bin/asm.o bin/errorHandl.o bin/keyboard.o bin/shell.o bin/memory.o bin/paging.o
	rm grub/boot/kernel.sys
	cp -i bin/Build/kernel.sys grub/boot/kernel.sys
	grub-mkrescue -o birdos.iso grub/
	virtualbox --startvm "Bird OS" --debug-command-line --start-running

modules:
	nasm modules/pVESA.asm -f bin -o modules/bin/VESA.SYS


run:
	#qemu-system-i386 -kernel bin/Build/kernel.sys
	virtualbox --startvm "Bird OS" --debug-command-line --start-running

setup:
	nasm boot/loader.asm -o $(SETUPLOC)loader.sys
	mkisofs -o $(SETUPLOC)test_setup.iso modules/bin/VESA.SYS $(SETUPLOC)main.bat \
	$(SETUPLOC)install.bat	 


