#include "common/common.h"
#include "interrupt/interrupt.h"
#include "syscall/syscall.h"

extern void trigger_syscall_exit(int32 exit_code);
extern int32 trigger_syscall_fork();
extern int32 trigger_syscall_exec(char* path, uint32 argc, char* argv[]);
extern void trigger_syscall_yield();

void exit(int32 exit_code) {
  return trigger_syscall_exit(exit_code);
}

int32 fork() {
  return trigger_syscall_fork();
}

int32 exec(char* path, uint32 argc, char* argv[]) {
  return trigger_syscall_exec(path, argc + 1, argv);
}

void yield() {
  return trigger_syscall_yield();
}
