# Release Note
# ------------
# 2012-02-14, gamja9e, first version

PROJECT := ganaeos64
OBJ = first.o
LINKER = gold
ASSEMBLER = as

$(PROJECT).bin: $(PROJECT).elf
	objcopy -Obinary $(PROJECT).elf $(PROJECT).bin

$(PROJECT).elf: $(OBJ)
	$(LINKER) -o $(PROJECT).elf -T ganaeos64.ld $(OBJ)

first.o: booting/first.S
	$(ASSEMBLER) -o first.o booting/first.S

clean:
	rm -rf $(PROJECT).bin $(PROJECT).elf $(OBJ)
