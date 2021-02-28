PROJDIRS := core

# called OBJDIR but is actually kernel.sys dir
OBJDIR   := bin
VM_NAME  := "BirdOS"

# any modules/objects not to be included in the final build
OBJIGNORE := core/misc/conway.o

SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES := $(shell find $(PROJDIRS) -type f -name "*.asm") #src/boot.asm #done to fix the can't find symbol start error
LDFILES  := $(shell find $(PROJDIRS) -type f -name "*.o")

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

ALLOBJFILES := $(foreach thing,$(SRCFILES),$(thing:%.c=%.o))
OBJFILES :=  $(filter-out $(OBJIGNORE), $(ALLOBJFILES))		#$(patsubst %.c, %.o, $(SRCFILES))
ASOBJFILES := $(foreach thing,$(ASMFILES),$(thing:%.asm=%.o))

LDOBJFILES := $(filter-out core/kernel.o, $(OBJFILES))
LDASOBJFILES := $(filter-out core/boot.o, $(ASOBJFILES))

ALLFILES := $(SRCFILES) $(HDRFILES) $(ASMFILES)

WARNINGS := -Wall -Wextra -pedantic -Wshadow \
	    -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wmissing-prototypes -Wmissing-declarations \
	    -Wredundant-decls -Wnested-externs -Winline \
	    -Wno-long-long -Wconversion -Wstrict-prototypes

CCFLAGS := -nostdlib -ffreestanding -g -std=c99 $(WARNINGS)
ASFLAGS := -w all -f elf32 #--fatal-warnings

CC := i686-elf-gcc
LD := i686-elf-ld
AC := nasm

.PHONY: all clean todo run assembly iso

iso: all
	cp bin/kernel.sys grub/boot/kernel.sys
	grub-mkrescue -o birdos.iso grub/

todo: 
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

all: clean $(OBJFILES) $(ASOBJFILES)
	@$(CC) -T linker.ld -o bin/kernel.sys core/boot.o core/kernel.o $(LDOBJFILES) $(LDASOBJFILES) -lgcc -ffreestanding -O2 -nostdlib

	@# let xenops update the BUILD version for next time
	-@xenops -f core/kernel/info.h -q

%.o: %.c
	@$(CC) $(CCFLAGS) -c $< -o $@

%.o: %.asm
	@$(AC) $(ASFLAGS) $< -o $@

clean:
	-@for file in $(DCLEAN:Makefile=); do rm $$file; done; true
	-@for file in $(OCLEAN:Makefile=); do rm $$file; done; true

# creates a map of all function
map:
	@$(LD) -Map=kernel.map -T linker.ld -o bin/kernel.sys core/boot.o core/kernel.o $(LDOBJFILES) $(LDASOBJFILES)

run:
	vboxmanage startvm $(VM_NAME) -E VBOX_GUI_DBG_ENABLED=true

# this one works better, because it start the command line on startup (instead
# of you having to manually open it, which is the case with the other one)
run-old:
	virtualbox --startvm $(VM_NAME) --debug-command-line --start-running