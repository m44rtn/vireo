PROJDIRS := src
OBJDIR   := bin


SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES :=  src/boot.asm #done to fix the can't find symbol start error
LDFILES  := $(shell find $(PROJDIRS) -type f -name "*.o")

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

OBJFILES := $(foreach thing,$(SRCFILES),$(thing:%.c=%.o)) #$(patsubst %.c, %.o, $(SRCFILES))
ASOBJFILES := $(foreach thing,$(ASMFILES),$(thing:%.asm=%.o))

LDOBJFILES := $(filter-out src/kernel.o, $(OBJFILES))
LDASOBJFILES := $(filter-out src/boot.o, $(ASOBJFILES))

ALLFILES := $(SRCFILES) $(HDRFILES) $(ASMFILES)

WARNINGS := -Wall -Wextra -pedantic -Wshadow \
	    -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wmissing-prototypes -Wmissing-declarations \
	    -Wredundant-decls -Wnested-externs -Winline \
	    -Wno-long-long -Wconversion -Wstrict-prototypes

CCFLAGS := -nostdlib -ffreestanding -g -O2 -std=c89 $(WARNINGS)
ASFLAGS := -w all -f elf32 #--fatal-warnings

CC := i686-elf-gcc
AC := nasm

.PHONY: all clean todo run assembly iso

todo: 
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

all: $(OBJFILES) $(ASOBJFILES)
	$(CC)  -T linker.ld -o bin/kernel.sys src/boot.o src/kernel.o $(LDOBJFILES) $(LDASOBJFILES) -lgcc -ffreestanding -O2 -nostdlib

	@# let xenops update the BUILD version for next time
	@xenops --file src/include/kernel_info.h 

%.o: %.c
	@echo $(OBJFILES)
	@echo $(SRCFILES)
	$(CC) $(CCFLAGS) -c $< -o $@
	#-c
	
	#-MMD -MP

%.o: %.asm
	$(AC) $(ASFLAGS) $< -o $@

clean:
	-@for file in $(DCLEAN:Makefile=); do rm $$file; done; true
	-@for file in $(OCLEAN:Makefile=); do rm $$file; done; true

iso:
	cp bin/kernel.sys grub/boot/kernel.sys
	grub-mkrescue -o birdos.iso grub/


run:
	virtualbox --startvm "Bird OS" --debug-command-line --start-running

