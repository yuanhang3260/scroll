SRC_DIR=src
OBJ_DIR=lib
BIN_DIR=bin

OBJS_C = \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/common/common.o \
	$(OBJ_DIR)/monitor/monitor.o \

OBJS_ASM = \

CC=gcc
ASM=nasm
CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -no-pie -fno-pic
IFLAGS=-I${SRC_DIR}
LDFLAGS=-m elf_i386 -Tlink.ld
ASFLAGS=-felf

all: image

image: mbr loader kernel
	rm -rf scroll.img && bximage -hd -mode="flat" -size=1 -q scroll.img 1>/dev/null
	dd if=$(BIN_DIR)/mbr of=scroll.img bs=512 count=1 seek=0 conv=notrunc
	dd if=$(BIN_DIR)/loader of=scroll.img bs=512 count=8 seek=1 conv=notrunc
	dd if=$(BIN_DIR)/kernel of=scroll.img bs=512 count=2048 seek=9 conv=notrunc

mbr: $(SRC_DIR)/boot/mbr.S
	mkdir -p $(BIN_DIR)
	nasm -o $(BIN_DIR)/mbr $<

loader: $(SRC_DIR)/boot/loader.S
	mkdir -p $(BIN_DIR)
	nasm -o $(BIN_DIR)/loader $<

kernel: ${OBJS_C} ${OBJS_ASM} link.ld
	ld $(LDFLAGS) -o bin/kernel ${OBJS_C} ${OBJS_ASM}

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.c
	mkdir -p $(OBJ_DIR)/common
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.c $(SRC_DIR)/common/%.h
	mkdir -p $(OBJ_DIR)/common
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/monitor/%.o: $(SRC_DIR)/monitor/%.c
	mkdir -p $(OBJ_DIR)/monitor
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/monitor/%.o: $(SRC_DIR)/monitor/%.c $(SRC_DIR)/monitor/%.h
	mkdir -p $(OBJ_DIR)/monitor
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(ASM) $(ASFLAGS) $< -o $@


clean:
	rm -rf ${OBJ_DIR}/* ${BIN_DIR}/* scroll.img bochsout.txt
