# Release Note
# ------------
# 2012-02-14, gamja9e, first version

PROJECT := ganaeos64
OBJ = first.o
LINKER = gold
ASSEMBLER = as
CC = gcc

$(PROJECT).bin: $(PROJECT).elf aligner
	objcopy -Obinary $(PROJECT).elf $(PROJECT).bin
	./out/host/bin/aligner $(PROJECT).bin 512

$(PROJECT).elf: $(OBJ)
	$(LINKER) -o $(PROJECT).elf -T ganaeos64.ld $(OBJ)

first.o: booting/first.S
	$(ASSEMBLER) -o first.o booting/first.S

clean:
	rm -rf $(PROJECT).bin $(PROJECT).elf $(OBJ)

aligner: tool/aligner/aligner.c
	$(CC) -o out/host/bin/aligner tool/aligner/aligner.c