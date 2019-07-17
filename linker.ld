OUTPUT_FORMAT("elf32-i386")
ENTRY(START)
/* STARTUP(src/boot.o)*/

OUTPUT(kernel.sys)

SECTIONS
{
    /*.text.start (_KERNEL_BASE_) : {
        src/boot.o( .text )
    }*/

    . = 1M;

    .text : ALIGN(0x1000){
       
        *(.multiboot)
        *(.text)
       
    }

    .data : ALIGN(0x1000){
        
        *(.data)
       
    }

    .bss : ALIGN(0x1000){
       
        *(.bss)
      
    }
}
