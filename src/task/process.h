#ifndef TASK_PROCESS_H
#define TASK_PROCESS_H

#include "task/thread.h"
#include "utils/bitmap.h"
#include "utils/linked_list.h"

#define USER_STACK_TOP   0xBFC00000  // 0xC0000000 - 4MB
#define USER_STACK_SIZE  65536       // 64KB
#define USER_PRCOESS_THREDS_MAX  4096

struct process_struct {
  uint32 id;
  char name[32];
  linked_list_t threads;
  // allocate user space thread for threads.
  bitmap_t user_thread_stack_indexes;
  // physical address of page directory.
  uint32 page_dir;
  // is kernel process?
  uint8 is_kernel_process;
};
typedef struct process_struct pcb_t;


// ****************************************************************************
struct process_struct* create_process(char* name, uint8 is_kernel_process);

struct task_struct* create_new_kernel_thread(
    pcb_t* process, char* name, void* function, char** argv);
struct task_struct* create_new_user_thread(
    pcb_t* process, char* name, void* user_function, uint32 argc, char** argv);

#endif
