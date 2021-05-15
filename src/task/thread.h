#ifndef TASK_THREAD_H
#define TASK_THREAD_H

#include "common/common.h"
#include "interrupt/interrupt.h"

#define KERNEL_MAIN_STACK_TOP    0xF0000000
#define THREAD_STACK_MAGIC       0x32602021

typedef isr_params_t interrupt_stack_t;

typedef void thread_func(void*);

enum task_status {
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
  TASK_WAITING,
  TASK_HANGING,
  TASK_DIED
};

struct task_struct {
  void* self_kstack;
  uint32 id;
  char name[32];
  uint8 priority;
  enum task_status status;
  uint32 ticks;
  uint32 stack_magic;  // This is the boundary of tcb_t and thread stack.
};
typedef struct task_struct tcb_t;

struct thread_stack {
  uint32 edi;
  uint32 esi;
  uint32 ebp;
  uint32 ebx;
  uint32 edx;
  uint32 ecx;
  uint32 eax;

  void (*eip)(thread_func* func, void* func_arg, tcb_t* thread);

  void (*unused_retaddr);
  thread_func* function;
  void* func_arg;
  tcb_t* tcb;
};
typedef struct thread_stack thread_stack_t;


// ****************************************************************************
// Create a new thread.
tcb_t* init_thread(char* name, thread_func function, void* func_arg, uint32 priority);

#endif
