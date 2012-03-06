/*
 * Release Note
 * ------------
 * 2012-02-16, gamja9e, first version
 * 2012-03-07, gamja9e, 2nd step of bootloader
 *   	                - load gtdr
 *	                    - jmp 32bit mode
 */
    .code16
    .section ".bltext", "ax"
    .global _bootloader_start
_bootloader_start:
    ljmp $0x7c0, $init_bootloader
init_bootloader:
    movw %cs, %ax
    movw %ax, %ss
    movw %ax, %ds
    movw %ax, %es
    movw $stack_top_position, %ax
    movw %ax, %sp
    cli
    movw $start_boot_msg, %si
    call print_msg

read_sectors_into_mem:
    movb $0x2, %ah
    movb $2, %al /* num of sectors */
    movb $0, %ch /* cylinder & 0xff */
    movb $2, %cl /* sector number 1 to 63 */
    movb $0, %dh /* head */
    movb $0, %dl /* 1st floppy */
    movw $0x200, %bx /* target offset */
    int $0x13
    jc none
		
    movw $copied_sectors_to_mem, %si
    call print_msg

jump_to_bootloader_main:
    jmp bootloader_main
	
print_msg:
    lodsb
    andb %al, %al
    jz _print_msg_end
    movb $0xe, %ah
    movw $0x7, %bx
    int $0x10
    jmp print_msg
_print_msg_end:	
    ret
	
none:
    hlt
    jmp none

    .section ".bldata", "a"
start_boot_msg:
    .ascii "Welcome to ganaeos64\n\r"
    .byte 0
copied_sectors_to_mem:
    .ascii "Read floppy sectors into memory\n\r"
    .byte 0
halt_the_machine:	
    .ascii "Halt the machine\n\r"
    .byte 0

    .section ".blmagic", "a"
stack_top_position:		
magic_number: .word 0xAA55

    .section ".blsec.text", "ax"
    .global bootloader_main
bootloader_main:
    movw $hello_bootloader_main, %si
    call print_msg

jump_32bit_mode:
    lgdt protect_32bit_gdtr
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0
    jmp clear_jump
clear_jump:	
    nop
    nop
    ljmp $0x8, $0x7c00 + main_32bit
	
    .section ".blsec.data", "a"
hello_bootloader_main:
    .ascii "Hello bootloader main\n\r"
    .byte 0

protect_32bit_gdtr:
    .word 0x1f
    .long 0x7c00 + protect_32bit_gdt

    .balign 8
protect_32bit_gdt:
    /* NULL gdt */
    .word 0x0
    .word 0x0
    .word 0x0
    .word 0x0
    /* Privilege mode, code gdt */
    .word 0xFFff /* limit low */
    .word 0x0000 /* base low */
    .byte 0x00   /* base middle */
    .byte 0x9a   /* 10011010b p, dpl, s, type */
    .byte 0xcf   /* 11001111b g, d/b, l, avl, limit high */
    .byte 0x00   /* base high */
     /* Privilege mode, data gdt */
    .word 0xFFff /* limit low */
    .word 0x0000 /* base low */
    .byte 0x00   /* base middle */
    .byte 0x92   /* 10010010b p, dpl, s, type */
    .byte 0xcf   /* 11001111b g, d/b, l, avl, limit high */
    .byte 0x00   /* base high */
		
    .code32
    .section ".blsec.text32", "ax"
main_32bit:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movl stack_top_position + 0x7c00, %eax
    movl %eax, %esp
	
    movl $0xb8000, %edi
    movb $'@', %bl
    movb %bl, (%edi)
none2:
	hlt
    jmp none2 + 0x7c00

	.end
		