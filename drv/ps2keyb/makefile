PROJDIRS := ./

# output file
OFILE := ps2keyb.drv

SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES := $(shell find $(PROJDIRS) -type f -name "*.asm")
LDFILES  := $(shell find $(PROJDIRS) -type f -name "*.o")

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

ALLOBJFILES := $(foreach thing,$(SRCFILES),$(thing:%.c=%.o))
OBJFILES :=  $(filter-out $(OBJIGNORE), $(ALLOBJFILES))	
ASOBJFILES := $(foreach thing,$(ASMFILES),$(thing:%.asm=%.o))

LDOBJFILES := $(OBJFILES)
LDASOBJFILES := $(ASOBJFILES)

ALLFILES := $(SRCFILES) $(HDRFILES) $(ASMFILES)

WARNINGS := -Wall -Wextra -pedantic -Wshadow \
	    -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wmissing-prototypes -Wmissing-declarations \
	    -Wredundant-decls -Wnested-externs -Winline \
	    -Wno-long-long -Wconversion -Wstrict-prototypes

CCFLAGS := -nostdlib -ffreestanding -g -std=c99 $(WARNINGS) -fpie -I../../syslib/lib/include
ASFLAGS := -w all -f elf32 #--fatal-warnings

CC := i686-elf-gcc
LD := i686-elf-ld
AC := nasm

.PHONY: all clean todo run assembly

all: clean $(OBJFILES) $(ASOBJFILES)
	@$(CC) -I../../syslib/lib/include -T linker.ld -o $(OFILE) $(LDOBJFILES) $(LDASOBJFILES) ../../syslib/bin/libvireo_sys.a -lgcc -pie -static $(CCFLAGS)


todo: 
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

%.o: %.c
	@$(CC) $(CCFLAGS) -c $< -o $@

%.o: %.asm
	@$(AC) $(ASFLAGS) $< -o $@

clean:
	-@for file in $(DCLEAN:Makefile=); do rm $$file; done; true
	-@for file in $(OCLEAN:Makefile=); do rm $$file; done; true

map:
	@$(LD) -Map=kernel.map -T linker.ld -o $(OFILE) $(LDOBJFILES) $(LDASOBJFILES) ../../syslib/bin/libvireo_sys.a
