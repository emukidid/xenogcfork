

     ENTRY(_start)
     SECTIONS
     {
       . = 0x81700000;
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

