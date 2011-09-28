

     ENTRY(_start)
     SECTIONS
     {
       . = 0x80003100;
       .text : { *(.text) }
       .data : { *(.data) }
       .sdata : { *(.sdata) }
       .rodata : { *(.rodata)}
       __bss_start = .;
       .bss : { *(.bss) }
       .sbss : { *(.sbss) }
       __bss_end = .;
       _end = .;
     }

