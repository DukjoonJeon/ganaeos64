OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(x86-64)
ENTRY(start_kernel)

SECTIONS
{
		.text 0x8000 : {
				*(.text.start)
			  	*(.text)
		}

		.eh_frame : {
			  	*(.eh_frame)
		}

		.rodata : {
			  	*(.rodata)
		}

		.data : {
			  	*(.data)
		}

		.bss : {
			  	*(.bss)
		}

		.debug_abbrev : {
			  *(.debug_abbrev)
		}

		.debug_info : {
		        *(.debug_info)
		}				

		.debug_line : {
                *(.debug_line)
        }

        .debug_loc : {
                *(.debug_log)
        }

        .debug_pubnames : {
		        *(.debug_pubnames)
        }

        .debug_aranges : {
		        *(.debug_aranges)
        }

        .debug_ranges : {
                *(.debug_ranges)
        }

        .debug_str : {
                *(.debug_str)
        }

		.comment : {
			  	*(.comment)
		}

		.note : {
			  	*(.note)
			  	*(.note.*)
		}
}
