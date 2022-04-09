PROJDIRS := lib/src lib/asm

# called OBJDIR but is actually kernel.sys dir
OBJDIR   := bin

OUTPUT := bin/libvireo_sys.a

SRCFILES = $(shell find $(PROJDIRS) -type f -name "*.c")
HDRFILES := $(shell find $(PROJDIRS) -type f -name "*.h")
ASMFILES := $(shell find $(PROJDIRS) -type f -name "*.asm") 
LDFILES  := $(shell find $(PROJDIRS) -type f -name "*.o")

DCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.d")
OCLEAN   := $(shell find $(PROJDIRS) -type f -name "*.o")

ALLOBJFILES := $(foreach thing,$(SRCFILES),$(thing:%.c=%.o))
OBJFILES :=  $(filter-out $(OBJIGNORE), $(ALLOBJFILES))		#$(patsubst %.c, %.o, $(SRCFILES))
ASOBJFILES := $(foreach thing,$(ASMFILES),$(thing:%.asm=%.o))

LDOBJFILES := $(filter-out core/main.o, $(OBJFILES))
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
AR := i686-elf-ar
AC := nasm

.PHONY: all clean todo run assembly iso

all: clean $(OBJFILES) $(ASOBJFILES)
	@$(AR) rcs $(OUTPUT) $(LDOBJFILES) $(LDASOBJFILES)

%.o: %.c
	@$(CC) $(CCFLAGS) -c $< -o $@

%.o: %.asm
	@$(AC) $(ASFLAGS) $< -o $@

clean:
	-@for file in $(DCLEAN:Makefile=); do rm $$file; done; true
	-@for file in $(OCLEAN:Makefile=); do rm $$file; done; true

todo: 
	-@for file in $(ALLFILES:Makefile=); do fgrep -H -e TODO -e FIXME $$file; done; true
