PROJECT := ganaeos64
FIRST_BL_OBJ := first.o
KERNEL_OBJ := main.o mm.o warn.o console.o delay.o
LINKER = gold
ASSEMBLER = as
CC = gcc

$(PROJECT).bin: $(PROJECT)_first_bl.bin $(PROJECT)_kernel.bin
	cp $(PROJECT)_first_bl.bin $(PROJECT).bin
	cat $(PROJECT)_kernel.bin >> $(PROJECT).bin

$(PROJECT)_first_bl.bin: $(PROJECT)_first_bl.elf aligner
	objcopy -Obinary $(PROJECT)_first_bl.elf $(PROJECT)_first_bl.bin
	./out/host/bin/aligner $(PROJECT)_first_bl.bin 512

$(PROJECT)_first_bl.elf: $(FIRST_BL_OBJ)
	$(LINKER) -o $(PROJECT)_first_bl.elf -T ganaeos64_first_bl.ld $(FIRST_BL_OBJ)

first.o: booting/first.S
	$(CC) -c booting/first.S -m64

###############################################################################

$(PROJECT)_kernel.bin: $(PROJECT)_kernel.elf
	objcopy -Obinary $< $@
	./out/host/bin/aligner $@ 512

$(PROJECT)_kernel.elf: $(KERNEL_OBJ)
	$(LINKER) -o $@ -T ganaeos64.ld $(KERNEL_OBJ)

main.o: kernel/main.c
	gcc -c $< -std=c99 -m64 -I./include -g

mm.o: kernel/mm.c
	gcc -c $< -std=c99 -m64 -I./include -g

warn.o: kernel/warn.c
	gcc -c $< -std=c99 -m64 -I./include -g

console.o: kernel/console.c
	gcc -c $< -std=c99 -m64 -I./include -g

delay.o: kernel/delay.c
	gcc -c $< -std=c99 -m64 -I./include -g

clean:
	rm -rf *.bin *.elf *.o

aligner: tool/aligner/aligner.c
	$(CC) -o out/host/bin/aligner tool/aligner/aligner.c