/*
 * Release Note
 * ------------
 * 2012-02-12, gamja9e, first version
 */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(x86-64)
ENTRY(bootloader_main)

SECTIONS
{
    . = 0;
    .bltext : {
        *(.bltext)
    }
    .bldata : {
        *(.bldata)
    }
    . = 510;
    .blmagic : {
        *(.blmagic)
    }

    /* already aligned by 512 */
    .blsec.text : {
        *(.blsec.text)
    }
    .blsec.data : {
        *(.blsec.data)
    }
	. = ALIGN(512);
    .blsec.text32 : {
        *(.blsec.text32)
    }

    .bss : {
        *(.bss)
    }
    .text : {
       *(.text)
       *(.text.*)
    }
    .data : {
 	*(data)
        *(data.*)
    }
    .note : {
        *(.note*)
    }
    . = ALIGN(512);
    _endof1stbl = .;
}
