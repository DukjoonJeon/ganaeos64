PROJECT := ganaeos64
FIRST_BL_OBJ = first.o
SECOND_BL_OBJ = global32bit.o set32bit.o
LINKER = gold
ASSEMBLER = as
CC = gcc

$(PROJECT).bin: $(PROJECT)_first_bl.bin #$(PROJECT)_second_bl.bin
	cp $(PROJECT)_first_bl.bin $(PROJECT).bin
#	cat $(PROJECT)_second_bl.bin >> $(PROJECT).bin

$(PROJECT)_second_bl.bin: $(PROJECT)_second_bl.elf
	objcopy -Obinary $(PROJECT)_second_bl.elf $(PROJECT)_second_bl.bin
	./out/host/bin/aligner $(PROJECT)_second_bl.bin 512

$(PROJECT)_second_bl.elf: $(SECOND_BL_OBJ)
	$(LINKER) -o $(PROJECT)_second_bl.elf -T ganaeos64_second_bl.ld $(SECOND_BL_OBJ)

$(PROJECT)_first_bl.bin: $(PROJECT)_first_bl.elf aligner
	objcopy -Obinary $(PROJECT)_first_bl.elf $(PROJECT)_first_bl.bin
	./out/host/bin/aligner $(PROJECT)_first_bl.bin 512

$(PROJECT)_first_bl.elf: $(FIRST_BL_OBJ)
	$(LINKER) -o $(PROJECT)_first_bl.elf -T ganaeos64_first_bl.ld $(FIRST_BL_OBJ)

first.o: booting/first.S
	$(CC) -c booting/first.S -m64

global32bit.o: booting/global32bit.c
	$(CC) -c booting/global32bit.c -std=c99 -m32 -I./booting

set32bit.o: booting/set32bit.c
	$(CC) -c booting/set32bit.c -std=c99 -m32 -I./booting

clean:
	rm -rf *.bin *.elf *.o

aligner: tool/aligner/aligner.c
	$(CC) -o out/host/bin/aligner tool/aligner/aligner.c