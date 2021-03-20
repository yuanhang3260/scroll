SRC_DIR=src
OBJ_DIR=lib
BIN_DIR=bin

OBJS_C = \
	$(OBJ_DIR)/main.o \

OBJS_ASM = \
	$(OBJ_DIR)/boot.o \

CC=gcc
ASM=nasm
CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector
IFLAGS=
LDFLAGS=-m elf_i386 -Tlink.ld
ASFLAGS=-felf

all: image

image: mbr loader
	rm -rf scroll.img && bximage -hd -mode="flat" -size=1 -q scroll.img 1>/dev/null
	dd if=$(BIN_DIR)/mbr of=scroll.img bs=512 count=1 seek=0 conv=notrunc
	dd if=$(BIN_DIR)/loader of=scroll.img bs=512 count=1 seek=1 conv=notrunc

mbr: $(SRC_DIR)/boot/mbr.S
	mkdir -p $(BIN_DIR)
	nasm -o $(BIN_DIR)/mbr $<

loader: $(SRC_DIR)/boot/loader.S
	mkdir -p $(BIN_DIR)
	nasm -o $(BIN_DIR)/loader $<

loader: $(SRC_DIR)/boot/loader.S

kernel: ${OBJS_C} ${OBJS_ASM} link.ld
	ld $(LDFLAGS) -o kernel ${OBJS_C} ${OBJS_ASM}

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(ASM) $(ASFLAGS) $< -o $@


clean:
	rm -rf ${OBJ_DIR}/*.o ${BIN_DIR}/* scroll.img bochsout.txt
