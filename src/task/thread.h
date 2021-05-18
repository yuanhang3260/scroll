#ifndef TASK_THREAD_H
#define TASK_THREAD_H

#include "common/common.h"
#include "interrupt/interrupt.h"
#include "task/process.h"

#define KERNEL_MAIN_STACK_TOP    0xF0000000
#define THREAD_STACK_MAGIC       0x32602021
#define THREAD_DEFAULT_PRIORITY  10

#define EFLAGS_MBS    (1 << 1)
#define EFLAGS_IF_0   (0 << 9)
#define EFLAGS_IF_1   (1 << 9)
#define EFLAGS_IOPL_0 (0 << 12)
#define EFLAGS_IOPL_3 (3 << 12)

typedef isr_params_t interrupt_stack_t;

typedef void thread_func(char**);

enum task_status {
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
  TASK_WAITING,
  TASK_HANGING,
  TASK_DIED
};

struct task_struct {
  // kernel stack pointer
  void* self_kstack;
  uint32 id;
  char name[32];
  uint8 priority;
  enum task_status status;
  // timer ticks this thread has been running for.
  uint32 ticks;
  // pointer to its process
  struct process_struct* process;
  // user stack
  uint32 user_stack;
  int32 user_stack_index;
  // syscall mark
  bool syscall_ret;
  // boundary of tcb_t and thread stack.
  uint32 stack_magic;
};
typedef struct task_struct tcb_t;

struct switch_stack {
  // Switch context.
  uint32 edi;
  uint32 esi;
  uint32 ebp;
  uint32 ebx;
  uint32 edx;
  uint32 ecx;
  uint32 eax;

  // Below are only used for thread first run, which enters kernel_thread.
  void (*eip)(thread_func* func, char** argv, tcb_t* thread);

  void (*unused_retaddr);
  thread_func* function;
  char** argv;
  tcb_t* tcb;
};
typedef struct switch_stack switch_stack_t;


// ****************************************************************************
// Create a new thread.
tcb_t* init_thread(tcb_t* thread, char* name, thread_func function, uint32 argc, char** argv,
    uint8 user_thread, uint32 priority);

uint32 prepare_user_stack(
    tcb_t* thread, uint32 stack_top, uint32 argc, char** argv, uint32 return_addr);

tcb_t* fork_crt_thread();

#endif
