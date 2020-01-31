PROJDIRS := core
OBJDIR   := bin


SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES := $(shell find $(PROJDIRS) -type f -name "*.asm") #src/boot.asm #done to fix the can't find symbol start error
LDFILES  := $(shell find $(PROJDIRS) -type f -name "*.o")

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

OBJFILES := $(foreach thing,$(SRCFILES),$(thing:%.c=%.o)) #$(patsubst %.c, %.o, $(SRCFILES))
ASOBJFILES := $(foreach thing,$(ASMFILES),$(thing:%.asm=%.o))

LDOBJFILES := $(filter-out core/kernel.o, $(OBJFILES))
LDASOBJFILES := $(filter-out core/boot.o, $(ASOBJFILES))

ALLFILES := $(SRCFILES) $(HDRFILES) $(ASMFILES)

WARNINGS := -Wall -Wextra -pedantic -Wshadow \
	    -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wmissing-prototypes -Wmissing-declarations \
	    -Wredundant-decls -Wnested-externs -Winline \
	    -Wno-long-long -Wconversion -Wstrict-prototypes

CCFLAGS := -nostdlib -ffreestanding -g -O2 -std=c89 $(WARNINGS)
ASFLAGS := -w all -f elf32 #--fatal-warnings

CC := i686-elf-gcc
LD := i686-elf-ld
AC := nasm

.PHONY: all clean todo run assembly iso

todo: 
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

all: clean $(OBJFILES) $(ASOBJFILES)
	@$(CC) -T linker.ld -o bin/kernel.sys core/boot.o core/kernel.o $(LDOBJFILES) $(LDASOBJFILES) -lgcc -ffreestanding -O2 -nostdlib

	@# let xenops update the BUILD version for next time
	@xenops --file core/include/kernel_info.h --quiet

%.o: %.c
	@$(CC) $(CCFLAGS) -c $< -o $@

%.o: %.asm
	@$(AC) $(ASFLAGS) $< -o $@

clean:
	-@for file in $(DCLEAN:Makefile=); do rm $$file; done; true
	-@for file in $(OCLEAN:Makefile=); do rm $$file; done; true

iso:
	cp bin/kernel.sys grub/boot/kernel.sys
	grub-mkrescue -o birdos.iso grub/

map:
	@$(LD) -Map=kernel.map -T linker.ld -o bin/kernel.sys core/boot.o core/kernel.o $(LDOBJFILES) $(LDASOBJFILES)

run:
	vboxmanage startvm "BirdOS" -E VBOX_GUI_DBG_ENABLED=true

run-old:
	virtualbox --startvm "BirdOS" --debug-command-line --start-running


