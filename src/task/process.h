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

  // tid -> threads;
  hash_table_t threads;

  // allocate user space thread for threads.
  bitmap_t user_thread_stack_indexes;

  // page directory.
  page_directory_t page_dir;

  // is kernel process?
  uint8 is_kernel_process;

  // waiting threads
  linked_list_t waiting_thread_nodes;

  // exit code map of processes that this process waits.
  hash_table_t wait_exit_codes;

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

void process_exit(pcb_t* process, int32 exit_code);
void destroy_process(pcb_t* process);

int32 process_fork();
int32 process_exec(char* path, uint32 argc, char* argv[]);
void process_wait(uint32 pid, uint32* status);

#endif
