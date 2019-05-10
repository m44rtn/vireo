OUTPUT_FORMAT(elf32-i386)
ENTRY(start /*setup32*/)
SECTIONS
 {
   /*. = 0x100000;*/
   /*//. = 0x0060;*/
   . = 0x200000; /*2 MiB*/
   .text : { *(.text) }
   .data : { *(.data) }
   .bss  : { *(.bss)  }

}
