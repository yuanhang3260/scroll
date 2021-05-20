#include "common/common.h"
#include "syscall/syscall.h"

extern void trigger_syscall_exit(int32 exit_code);
extern int32 trigger_syscall_fork();
extern int32 trigger_syscall_exec(char* path, uint32 argc, char* argv[]);
extern void trigger_syscall_yield();
extern int32 trigger_syscall_read(char* filename, char* buffer, uint32 offset, uint32 size);
extern int32 trigger_syscall_write(char* filename, char* buffer, uint32 offset, uint32 size);
extern int32 trigger_syscall_stat(char* filename, file_stat_t* stat);
extern void trigger_syscall_print(char* str, void* args);

void exit(int32 exit_code) {
  return trigger_syscall_exit(exit_code);
}

int32 fork() {
  return trigger_syscall_fork();
}

int32 exec(char* path, uint32 argc, char* argv[]) {
  return trigger_syscall_exec(path, argc, argv);
}

void yield() {
  return trigger_syscall_yield();
}

int32 read(char* filename, char* buffer, uint32 offset, uint32 size) {
  return trigger_syscall_read(filename, buffer, offset, size);
}

int32 write(char* filename, char* buffer, uint32 offset, uint32 size) {
  return trigger_syscall_write(filename, buffer, offset, size);
}

int32 stat(char* filename, file_stat_t* stat) {
  return trigger_syscall_stat(filename, stat);
}

void print(char* str, void* args) {
  return trigger_syscall_print(str, args);
}