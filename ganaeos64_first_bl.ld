/*
 * Release Note
 * ------------
 * 2012-02-12, gamja9e, first version
 */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(x86-64)
ENTRY(bootloader_start)

SECTIONS
{
    . = 0;
    .bl.text16 : {
        *(.bl.text16)
    }
    .bl.data16 : {
        *(.bl.data16)
    }
    . = 510;
    .bl.magic : {
        *(.bl.magic)
    }

    /* already aligned by 512 */
    .bl.text64 : {
        *(.bl.text64)
    }

	.bl.data64 : {
	    *(.bl.data64)
    }

    .note : {
        *(.note*)
    }

	
}
