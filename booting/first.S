/*
 * Release Note
 * ------------
 * 2012-02-16, gamja9e, first version
 */
    .code16
    .section ".bltext", "ax"
    .global _bootloader_start
_bootloader_main:
    ljmp $0x7c0, $init_bootloader
init_bootloader:
    movw %cs, %ax
    movw %ax, %ss
    movw %ax, %ds
    movw %ax, %es
    xorw %sp, %sp
    sti
    movw $start_boot_msg, %si
    call print_msg

read_sectors_into_mem:
    movb $0x2, %ah
    movb $2, %al /* num of sectors */
    movb $0, %ch /* cylinder */
    movb $1, %cl /* sector number */
    movw $0, %dx
    movw $512, %bx /* target offset */
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
magic_number: .word 0xAA55

    .section ".blsec.text", "ax"
    .global bootloader_main
bootloader_main:
    movw $hello_bootloader_main, %si
    call print_msg
    jmp none

    .section ".blsec.data", "a"
hello_bootloader_main:
    .ascii "Hello bootloader main\n\r"
    .byte 0