PROJDIRS := src
OBJDIR   := bin

SRCFILES := $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES := src/boot.asm #done to fix the can't find symbol start error
LDFILES  := $(shell find $(PROJDIRS) -type f -name "*.o")

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

OBJFILES := $(patsubst %.c, %.o, $(SRCFILES))
ASOBJFILES := $(patsubst %.asm, %.o, $(ASMFILES))

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

all: $(OBJFILES)
	$(CC)  -T linker.ld -o bin/kernel.sys src/boot.o src/kernel.o $(LDOBJFILES) $(LDASOBJFILES) -lgcc -ffreestanding -O2 -nostdlib

	@# let xenops update the BUILD version for next time
	@xenops --file src/include/kernel_info.h 

$(OBJFILES): $(SRCFILES) makefile $(ASOBJFILES)
	@$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@

$(ASOBJFILES): $(ASMFILES) makefile 
	@$(AC) $(ASFLAGS) $< -o $@

clean:
	-@for file in $(DCLEAN:Makefile=); do rm $$file; done; true
	-@for file in $(OCLEAN:Makefile=); do rm $$file; done; true

iso:
	cp bin/kernel.sys grub/boot/kernel.sys
	grub-mkrescue -o birdos.iso grub/


run:
	virtualbox --startvm "Bird OS" --debug-command-line --start-running


