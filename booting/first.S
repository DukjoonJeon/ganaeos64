/*
 * Release Note
 * ------------
 * 2012-02-16, gamja9e, first version
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
    sti
    movw $start_boot_msg, %si
    call print_msg

read_sectors_into_mem:
    movb $0x2, %ah
    movb $2, %al /* num of sectors */
    movb $0, %ch /* track */
    movb $2, %cl /* sector number 1 to 63 */
    movb $0, %dh /* head */
    movb $0, %dl /* 1st floppy */
    movw $0x200, %bx /* target offset */
    int $0x13
    jae none
		
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
    movw $copied_sectors_to_mem, %si
    call print_msg
_none:	
    hlt
    jmp _none

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
none2:	
    jmp none2

    .section ".blsec.data", "a"
hello_bootloader_main:
    .ascii "Hello bootloader main\n\r"
    .byte 0
