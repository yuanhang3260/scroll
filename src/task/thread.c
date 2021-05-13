#include "monitor/monitor.h"
#include "task/thread.h"
#include "task/scheduler.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "common/stdlib.h"

extern uint32 get_esp();

static uint32 next_thread_id = 1;

static void kernel_thread(thread_func* function, void* func_arg, tcb_t* thread) {
  function(func_arg);

  // Destroy this thread and release all its resources.
  thread_destroy(thread);
}

tcb_t* init_thread(char* name, thread_func function, void* func_arg, uint32 priority) {
  // Allocate one page as tcb_t and kernel stack for each thread.
  tcb_t* thread = (tcb_t*)kmalloc_aligned(PAGE_SIZE);
  map_page((uint32)thread);
  memset(thread, 0, sizeof(tcb_t));

  thread->id = next_thread_id++;
  if (name != 0) {
    strcpy(thread->name, name);
  } else {
    char buf[32];
    sprintf(buf, "thread-%u", thread->id);
    strcpy(thread->name, buf);
  }

  thread->status = TASK_RUNNING;
  thread->ticks = 0;
  thread->priority = priority;
  thread->stack_magic = THREAD_STACK_MAGIC;

  // Init thread stack.
  thread->self_kstack =
      (void*)thread + PAGE_SIZE - (sizeof(interrupt_stack_t) + sizeof(thread_stack_t));
  thread_stack_t* kthread_stack = (thread_stack_t*)thread->self_kstack;

  kthread_stack->edi = 0;
  kthread_stack->esi = 0;
  kthread_stack->ebp = 0;
  kthread_stack->ebx = 0;
  kthread_stack->edx = 0;
  kthread_stack->ecx = 0;
  kthread_stack->eax = 0;

  kthread_stack->eip = kernel_thread;
  kthread_stack->function = function;
  kthread_stack->func_arg = func_arg;
  kthread_stack->tcb = thread;

  return thread;
}
