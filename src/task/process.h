#ifndef TASK_PROCESS_H
#define TASK_PROCESS_H

#include "task/thread.h"
#include "mem/paging.h"
#include "sync/mutex.h"
#include "sync/spinlock.h"
#include "utils/bitmap.h"
#include "utils/linked_list.h"
#include "utils/hash_table.h"

#define USER_STACK_TOP   0xBFC00000  // 0xC0000000 - 4MB
#define USER_STACK_SIZE  65536       // 64KB
#define USER_PRCOESS_THREDS_MAX  4096

struct process_struct {
  uint32 id;
  char name[32];

  uint32 parent_pid;

  // tid -> threads;
  hash_table_t threads;

  // allocate user space thread for threads.
  bitmap_t user_thread_stack_indexes;

  // page directory.
  page_directory_t page_dir;

  // is kernel process?
  uint8 is_kernel_process;

  // exit code of child processes.
  hash_table_t children_exit_codes;

  linked_list_t waiting_thread_nodes;

  // lock to protect this struct
  spinlock_t lock;
};
typedef struct process_struct pcb_t;


// ****************************************************************************
struct process_struct* create_process(char* name, uint8 is_kernel_process);

struct task_struct* create_new_kernel_thread(
    pcb_t* process, char* name, void* function, char** argv);
struct task_struct* create_new_user_thread(
    pcb_t* process, char* name, void* user_function, uint32 argc, char** argv);

void add_process_thread(pcb_t* process, struct task_struct* thread);
void remove_process_thread(pcb_t* process, struct task_struct* thread);

void exit_process(pcb_t* process, int32 exit_code);
void destroy_process(pcb_t* process);

// syscalls implementation
int32 process_fork();
int32 process_exec(char* path, uint32 argc, char* argv[]);
int32 process_wait(uint32 pid, uint32* status);

#endif
