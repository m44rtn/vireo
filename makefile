PROJDIRS := src 
OBJDIR   := bin

SRCFILES := $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES := $(shell find $(PROJDIRS) -type f -name "*.s")
LDFILES  := $(filter-out src/boot.o, $(filter-out src/kernel.o, $(shell find $(PROJDIRS) -type f -name "*.o")))

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

OBJFILES := $(patsubst %.c, %.o, $(SRCFILES))
ASOBJFILES := $(patsubst %.s, %.o, $(ASMFILES))

ALLFILES := $(SRCFILES) $(HDRFILES) $(ASMFILES)

WARNINGS := -Wall -Wextra -pedantic -Wshadow \
	    -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wmissing-prototypes -Wmissing-declarations \
	    -Wredundant-decls -Wnested-externs -Winline \
	    -Wno-long-long -Wconversion -Wstrict-prototypes

CCFLAGS := -ffreestanding -g -O2 -std=c89 $(WARNINGS)
ASFLAGS := --warn #--fatal-warnings

CC := i686-elf-gcc
AC := i686-elf-as

.PHONY: all clean todo run assembly kernel iso

todo: 
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true

all: $(OBJFILES)
	@$(CC)  -T linker.ld -o obj/kernel.sys -ffreestanding -O2 -nostdlib src/boot.o src/kernel.o $(LDFILES) -lgcc
	
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
	cp obj/kernel.sys grub/kernel.sys
	grub-mkrescue -o birdos.iso grub/


run:
	virtualbox --startvm "Bird OS" --debug-command-line --start-running


