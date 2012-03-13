OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(x86-64)
ENTRY(prepair_jumping_64bit)

SECTIONS
{
    /* hardcoded address */
    .text : AT(0x8200) {
        *(.text)
        *(.text.*)
    }

    .data : {
 	*(.data)
        *(.data.*)
    }

    .bss : {
 	*(.bss)
        *(.bss.*)
    }

    .note : {
        *(.note*)
    }
    
    .comment : {
        *(.comment)
    }
}