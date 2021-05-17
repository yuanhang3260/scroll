#ifndef SYSCALL_SYSCALL_H
#define SYSCALL_SYSCALL_H

#include "common/common.h"
#include "interrupt/interrupt.h"

void exit(int32 exit_code);

int32 fork();

int32 exec(char* path, uint32 argc, char* argv[]);

void yield();

#endif
