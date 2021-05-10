#include "monitor/monitor.h"
#include "task/thread.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "common/stdlib.h"

static void kernel_thread(thread_func* function, void* func_arg) {
  function(func_arg);
}

static void init_thread(tcb* pthread, char* name, uint32 prio) {
  memset(pthread, 0, sizeof(tcb));
  strcpy(pthread->name, name);
  pthread->status = TASK_RUNNING;
  pthread->priority = prio;
  pthread->self_kstack = (void*)pthread + PAGE_SIZE;
  pthread->stack_magic = THREAD_STACK_MAGIC;
}

static void thread_create(tcb* pthread, thread_func function, void* func_arg) {
  pthread->self_kstack -= sizeof(interrupt_stack_t);
  pthread->self_kstack -= sizeof(thread_stack_t);
  thread_stack_t* kthread_stack = (thread_stack_t*)pthread->self_kstack;

  kthread_stack->eip = kernel_thread;
  kthread_stack->function = function;
  kthread_stack->func_arg = func_arg;

  kthread_stack->ebp = 0;
  kthread_stack->ebx = 0;
  kthread_stack->esi = 0;
  kthread_stack->edi = 0;
}

tcb* thread_start(char* name, uint32 priority, thread_func function, void* func_arg) {
  // Allocate one page as tcb and kernel stack for each thread.
  tcb* thread = (tcb*)kmalloc_aligned(PAGE_SIZE);
  map_page((uint32)thread);

  init_thread(thread, name, priority);
  thread_create(thread, function, func_arg);

  asm volatile ("movl %0, %%esp; \
    pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; \
    ret": : "g" (thread->self_kstack) : "memory");
  return thread;
}
