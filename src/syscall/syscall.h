#ifndef SYSCALL_SYSCALL_H
#define SYSCALL_SYSCALL_H

#include "common/common.h"
#include "fs/file.h"

void exit(int32 exit_code);

int32 fork();

int32 exec(char* path, uint32 argc, char* argv[]);

void yield();

int32 read(char* filename, char* buffer, uint32 offset, uint32 size);

int32 write(char* filename, char* buffer, uint32 offset, uint32 size);

int32 stat(char* filename, file_stat_t* stat);

int32 listdir(char* dir);

void print(char* str, void* args);

void wait(uint32 pid, uint32* status);

#endif
