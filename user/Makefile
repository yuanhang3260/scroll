SRC_DIR=./src
LIB_DIR=./lib
BIN_DIR=./progs

SYS_INC_DIR=../src
SYS_LIB_DIR=../lib

CC=gcc -Werror
CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -no-pie -fno-pic
IFLAGS=-I${SRC_DIR} -I${SYS_INC_DIR}
LDFLAGS=-m elf_i386 -Tlink.ld

SYS_OBJS = \
	$(SYS_LIB_DIR)/common/common.o \
	$(SYS_LIB_DIR)/common/stdio.o \
	$(SYS_LIB_DIR)/common/util.o \
	$(SYS_LIB_DIR)/syscall/syscall.o \
	$(SYS_LIB_DIR)/syscall/syscall_trigger.o \
	$(SYS_LIB_DIR)/utils/math.o \
	$(SYS_LIB_DIR)/fs/file.o \
	$(LIB_DIR)/sys/common.o

PROGS = \
  ${BIN_DIR}/hello \
  ${BIN_DIR}/shell \
  ${BIN_DIR}/fork_bomb \
  ${BIN_DIR}/cat \
  ${BIN_DIR}/ls \

all: prepare image

prepare: ${SRC_DIR}/*
	mkdir -p $(BIN_DIR)
	mkdir -p $(LIB_DIR)

image: progs
	./disk_image_writer

progs: ${PROGS}

$(BIN_DIR)/%: $(LIB_DIR)/%.o ${SYS_OBJS}
	ld $(LDFLAGS) $< ${SYS_OBJS} -o $@

$(LIB_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(LIB_DIR)/sys/%.o: $(SRC_DIR)/sys/%.c $(SRC_DIR)/sys/%.h
	mkdir -p $(LIB_DIR)/sys
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	rm -rf ${BIN_DIR}/* ${LIB_DIR}/* user_disk_image
	cp greeting.txt ${BIN_DIR}/greeting.txt
