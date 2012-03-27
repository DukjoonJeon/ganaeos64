#define PML4_SEG   0x900
#define PDPTE_SEG  0xA00
#define PDE_SEG    0xB00
#define PTE_SEG    0xC00 /* 0 ~ 2mbyte */
#define PTE_SEG2   0xD00 /* 2 ~ 4mbyte */
#define KERNEL_SEG 0xE00
	
	.code16
	.section ".bl.text16", "ax"
	.global bootloader_start
	.extern main_64bit
bootloader_start:
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
	
	/* enable a20 pin for using over 1mb memory */
	movw $0x2401, %ax
	int $0x15
	jc none
	
	/* read floppy's sectors into mem */
	movb $0x2, %ah
	movb $10, %al /* num of sectors */
	movb $0, %ch /* cylinder & 0xff */
	movb $2, %cl /* sector number 1 to 63 */
	movb $0, %dh /* head */
	movb $0, %dl /* 1st floppy */
	movw $0x200, %bx /* target offset */
	int $0x13
	jc none
	
	movw $copied_sectors_to_mem, %si
	call print_msg
	
	/* load gdt */
	lgdt protect_64bit_gdtr
	
	/* fill page table */
	movw $0, %si	
	/* set PML4 */
	movw $PML4_SEG, %ax
	movw %ax, %es
	movw $PDPTE_SEG << 4, %bx
	addw $7, %bx
	movw %bx, %es:(%si)
	/* set PDPTE */
	movw $PDPTE_SEG, %ax
	movw %ax, %es
	movw $PDE_SEG << 4, %bx
	addw $7, %bx
	movw %bx, %es:(%si)
	/* set PDE */
	movw $PDE_SEG, %ax
	movw %ax, %es
	movw $PTE_SEG << 4, %bx
	addw $7, %bx
	movw %bx, %es:(%si)
	/* set PDE2 */
	movw $PTE_SEG2 << 4, %bx
	addw $7, %bx
	addw $8, %si
	movw %bx, %es:(%si)
	/* set PTE */
	movw $PTE_SEG, %ax
	movw %ax, %es
	movl $7, %eax /* P, R/W, U/S */
	movw $1024, %cx
	movw $0, %si
set_pte:	
	movl %eax, %es:(%si)
	addw $8, %si
	addl $4096, %eax
	loop set_pte
	
	movw $end_fill_pte_msg, %si
	call print_msg
	
	/* PAE enable */
	movl %cr4, %eax
	orl $(1 << 5), %eax
	movl %eax, %cr4
	
	/* EFER.MSR.LM enable */
	movl $0xC0000080, %ecx
	rdmsr
	orl $(1 << 8), %eax
	wrmsr
	
	/* register pml4 */
	movl $PML4_SEG << 4, %eax
	movl %eax, %cr3
	
	/* enable paging and pm */
	movl %cr0, %eax
	orl $0x80000001, %eax
	movl %eax, %cr0
	
	ljmp $0x8, $0x7c00 + main_64bit		
	
none:	
 	movw $halt_the_machine, %si
 	call print_msg
_none:		
	hlt
	jmp _none
	
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

	.section ".bl.data16", "a"
start_boot_msg:
	.ascii "Welcome to ganaeos64\n\r"
	.byte 0
end_fill_pte_msg:
	.ascii "Welcome to ganaeos64\n\r"
	.byte 0
copied_sectors_to_mem:
	.ascii "Read floppy sectors into memory\n\r"
	.byte 0
halt_the_machine:	
	.ascii "Halt the machine\n\r"
	.byte 0

protect_64bit_gdtr:
	.word 0x17
	.long 0x7c00 + protect_64bit_gdt

	.balign 8
protect_64bit_gdt:
	.word 0x0000
	.word 0x0000
	.word 0x0000
	.word 0x0000
	/* 64bit code for privilege mode */
	.word 0x0000
	.word 0x0000
	.byte 0x00   
	.byte 0x98   
	.byte 0x20   
	.byte 0x00   
	/* 64bit data for privilege mode */
	.word 0x0000 
	.word 0x0000 
	.byte 0x00   
	.byte 0x90   
	.byte 0x00   
	.byte 0x00   		
	
	.section ".bl.magic", "a"
stack_top_position:		
magic_number: .word 0xAA55

	.code64
	.section ".bl.text64", "ax"
	.global main_64bit
main_64bit:
	movw $0xf, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	
	movq $stack_top_position + 0x7c00, %rsp
	
none2:			
	jmp none2
	.section ".bl.data64", "a"
none3: