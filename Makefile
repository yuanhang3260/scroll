SRC_DIR=src
OBJ_DIR=lib
BIN_DIR=bin
USER_DIR=user

OBJS_C = \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/common/common.o \
	$(OBJ_DIR)/common/io.o \
	$(OBJ_DIR)/common/stdlib.o \
	$(OBJ_DIR)/common/stdio.o \
	$(OBJ_DIR)/common/global.o \
	$(OBJ_DIR)/common/util.o \
	$(OBJ_DIR)/monitor/monitor.o \
	$(OBJ_DIR)/interrupt/interrupt.o \
	$(OBJ_DIR)/interrupt/idt.o \
	$(OBJ_DIR)/interrupt/timer.o \
	$(OBJ_DIR)/mem/gdt.o \
	$(OBJ_DIR)/mem/gdt_load.o \
	$(OBJ_DIR)/mem/paging.o \
	$(OBJ_DIR)/mem/kheap.o \
	$(OBJ_DIR)/task/thread.o \
	$(OBJ_DIR)/task/process.o \
	$(OBJ_DIR)/task/scheduler.o \
	$(OBJ_DIR)/task/schedule.o \
	$(OBJ_DIR)/syscall/syscall_wrapper.o \
	$(OBJ_DIR)/syscall/syscall_impl.o \
	$(OBJ_DIR)/syscall/syscall.o \
	$(OBJ_DIR)/syscall/syscall_trigger.o \
	$(OBJ_DIR)/sync/cas.o \
	$(OBJ_DIR)/sync/spinlock.o \
	$(OBJ_DIR)/sync/mutex.o \
	$(OBJ_DIR)/sync/cond_var.o \
	$(OBJ_DIR)/fs/disk_io.o \
	$(OBJ_DIR)/fs/fs.o \
	$(OBJ_DIR)/fs/file.o \
	$(OBJ_DIR)/fs/naive_fs.o \
	$(OBJ_DIR)/elf/elf.o \
	$(OBJ_DIR)/driver/hard_disk.o \
	$(OBJ_DIR)/driver/keyboard.o \
	$(OBJ_DIR)/driver/keyhelp.o \
	$(OBJ_DIR)/utils/debug.o \
	$(OBJ_DIR)/utils/bitmap.o \
	$(OBJ_DIR)/utils/ordered_array.o \
	$(OBJ_DIR)/utils/math.o \
	$(OBJ_DIR)/utils/rand.o \
	$(OBJ_DIR)/utils/linked_list.o \
	$(OBJ_DIR)/utils/hash_table.o \
	$(OBJ_DIR)/utils/string.o \

OBJS_ASM = \

CC=gcc -Werror
ASM=nasm
CFLAGS=-m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -no-pie -fno-pic
IFLAGS=-I${SRC_DIR}
LDFLAGS=-m elf_i386 -Tlink.ld
ASFLAGS=-felf

all: image

prepare: ${SRC_DIR}/*
	mkdir -p $(BIN_DIR)
	mkdir -p $(OBJ_DIR)

image: prepare mbr loader kernel disk
	rm -rf scroll.img && bximage -hd -mode="flat" -size=3 -q scroll.img 1>/dev/null
	dd if=$(BIN_DIR)/mbr of=scroll.img bs=512 count=1 seek=0 conv=notrunc
	dd if=$(BIN_DIR)/loader of=scroll.img bs=512 count=8 seek=1 conv=notrunc
	dd if=$(BIN_DIR)/kernel of=scroll.img bs=512 count=2048 seek=9 conv=notrunc
	dd if=$(USER_DIR)/user_disk_image of=scroll.img bs=512 count=2048 seek=2057 conv=notrunc

mbr: $(SRC_DIR)/boot/mbr.S
	nasm -o $(BIN_DIR)/mbr $<

loader: $(SRC_DIR)/boot/loader.S
	nasm -o $(BIN_DIR)/loader $<

kernel: ${OBJS_C} ${OBJS_ASM} link.ld
	ld $(LDFLAGS) -o bin/kernel ${OBJS_C} ${OBJS_ASM}

disk: user_progs

user_progs: ./${USER_DIR}/src
	cd ./${USER_DIR} && make

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.c
	mkdir -p $(OBJ_DIR)/common
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.c $(SRC_DIR)/common/%.h
	mkdir -p $(OBJ_DIR)/common
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/common/%.o: $(SRC_DIR)/common/%.S
	mkdir -p $(OBJ_DIR)/interrupt
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/monitor/%.o: $(SRC_DIR)/monitor/%.c
	mkdir -p $(OBJ_DIR)/monitor
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/monitor/%.o: $(SRC_DIR)/monitor/%.c $(SRC_DIR)/monitor/%.h
	mkdir -p $(OBJ_DIR)/monitor
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/interrupt/%.o: $(SRC_DIR)/interrupt/%.c
	mkdir -p $(OBJ_DIR)/interrupt
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/interrupt/%.o: $(SRC_DIR)/interrupt/%.c $(SRC_DIR)/interrupt/%.h
	mkdir -p $(OBJ_DIR)/interrupt
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/interrupt/%.o: $(SRC_DIR)/interrupt/%.S
	mkdir -p $(OBJ_DIR)/interrupt
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/mem/%.o: $(SRC_DIR)/mem/%.c
	mkdir -p $(OBJ_DIR)/mem
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/mem/%.o: $(SRC_DIR)/mem/%.c $(SRC_DIR)/mem/%.h
	mkdir -p $(OBJ_DIR)/mem
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/mem/%.o: $(SRC_DIR)/mem/%.S
	mkdir -p $(OBJ_DIR)/mem
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/task/%.o: $(SRC_DIR)/task/%.c
	mkdir -p $(OBJ_DIR)/task
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/task/%.o: $(SRC_DIR)/task/%.c $(SRC_DIR)/task/%.h
	mkdir -p $(OBJ_DIR)/task
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/task/%.o: $(SRC_DIR)/task/%.S
	mkdir -p $(OBJ_DIR)/task
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/syscall/%.o: $(SRC_DIR)/syscall/%.c
	mkdir -p $(OBJ_DIR)/syscall
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/syscall/%.o: $(SRC_DIR)/syscall/%.c $(SRC_DIR)/syscall/%.h
	mkdir -p $(OBJ_DIR)/syscall
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/syscall/%.o: $(SRC_DIR)/syscall/%.S
	mkdir -p $(OBJ_DIR)/syscall
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/sync/%.o: $(SRC_DIR)/sync/%.c
	mkdir -p $(OBJ_DIR)/sync
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/sync/%.o: $(SRC_DIR)/sync/%.c $(SRC_DIR)/sync/%.h
	mkdir -p $(OBJ_DIR)/sync
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/sync/%.o: $(SRC_DIR)/sync/%.S
	mkdir -p $(OBJ_DIR)/sync
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/fs/%.o: $(SRC_DIR)/fs/%.c
	mkdir -p $(OBJ_DIR)/fs
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/fs/%.o: $(SRC_DIR)/fs/%.c $(SRC_DIR)/fs/%.h
	mkdir -p $(OBJ_DIR)/fs
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/fs/%.o: $(SRC_DIR)/fs/%.S
	mkdir -p $(OBJ_DIR)/fs
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/elf/%.o: $(SRC_DIR)/elf/%.c
	mkdir -p $(OBJ_DIR)/elf
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/elf/%.o: $(SRC_DIR)/elf/%.c $(SRC_DIR)/elf/%.h
	mkdir -p $(OBJ_DIR)/elf
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/elf/%.o: $(SRC_DIR)/elf/%.S
	mkdir -p $(OBJ_DIR)/elf
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/driver/%.o: $(SRC_DIR)/driver/%.c
	mkdir -p $(OBJ_DIR)/driver
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/driver/%.o: $(SRC_DIR)/driver/%.c $(SRC_DIR)/driver/%.h
	mkdir -p $(OBJ_DIR)/driver
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/driver/%.o: $(SRC_DIR)/driver/%.S
	mkdir -p $(OBJ_DIR)/driver
	$(ASM) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/utils/%.o: $(SRC_DIR)/utils/%.c
	mkdir -p $(OBJ_DIR)/utils
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/utils/%.o: $(SRC_DIR)/utils/%.c $(SRC_DIR)/utils/%.h
	mkdir -p $(OBJ_DIR)/utils
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR)/utils/%.o: $(SRC_DIR)/utils/%.S
	mkdir -p $(OBJ_DIR)/utils
	$(ASM) $(ASFLAGS) $< -o $@


clean:
	rm -rf ${OBJ_DIR}/* ${BIN_DIR}/* scroll.img bochsout.txt kernel_dump.txt
	cd ./${USER_DIR} && make clean
