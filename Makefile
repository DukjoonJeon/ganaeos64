PROJECT := ganaeos64
FIRST_BL_OBJ = first.osa
LINKER = gold
ASSEMBLER = as
CC = gcc

$(PROJECT).bin: $(PROJECT)_first_bl.bin #$(PROJECT)_second_bl.bin
	cp $(PROJECT)_first_bl.bin $(PROJECT).bin

$(PROJECT)_first_bl.bin: $(PROJECT)_first_bl.elf aligner
	objcopy -Obinary $(PROJECT)_first_bl.elf $(PROJECT)_first_bl.bin
	./out/host/bin/aligner $(PROJECT)_first_bl.bin 512

$(PROJECT)_first_bl.elf: $(FIRST_BL_OBJ)
	$(LINKER) -o $(PROJECT)_first_bl.elf -T ganaeos64_first_bl.ld $(FIRST_BL_OBJ)

first.o: booting/first.S
	$(CC) -c booting/first.S -m64


clean:
	rm -rf *.bin *.elf *.o

aligner: tool/aligner/aligner.c
	$(CC) -o out/host/bin/aligner tool/aligner/aligner.c