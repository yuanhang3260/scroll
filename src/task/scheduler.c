#include "task/thread.h"
#include "task/scheduler.h"
#include "interrupt/interrupt.h"
#include "mem/kheap.h"
#include "utils/linked_list.h"
#include "utils/debug.h"

extern void cpu_idle();
extern void context_switch(tcb* crt, tcb* next);

static tcb* main_thread;
static tcb* crt_thread;

static linked_list_t ready_tasks;
static linked_list_t blocking_tasks;

static void cpu_idle_thread() {
  while (1) {
    cpu_idle();
  }
}

static void thread_start(tcb* thread) {
  asm volatile (
   "movl %0, %%esp; \
    pop %%ebp; \
    pop %%ebx; \
    pop %%edi; \
    pop %%esi; \
    ret": : "g" (thread->self_kstack) : "memory");
}

void init_scheduler() {
  // Init task queues.
  ready_tasks = create_linked_list();
  blocking_tasks = create_linked_list();

  // Create kernel main thread - cpu_idle thread.
  main_thread = init_thread("kernel_main", cpu_idle_thread, (void*)0, 10);
  crt_thread = main_thread;
}

void start_scheduler() {
  thread_start(main_thread);
  // Never should reach here!
  PANIC();
}

tcb* create_thread(char* name, thread_func function, void* func_arg, uint32 priority) {
  if (priority == 0) {
    priority = 10;
  }
  tcb* thread = init_thread(name, function, func_arg, priority);
  linked_list_node_t* node = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  node->ptr = (void*)thread;
  disable_interrupt();
  linked_list_append(&ready_tasks, node);
  enable_interrupt();

  return thread;
}

static void do_context_switch() {
  // Context switch to next thread.
  linked_list_node_t* head = ready_tasks.head;
  linked_list_remove(&ready_tasks, head);
  tcb* old_thread = crt_thread;
  tcb* next_thread = (tcb*)head->ptr;

  if (old_thread != main_thread) {
    head->ptr = (void*)old_thread;
    linked_list_append(&ready_tasks, head);
  }
  crt_thread = next_thread;

  context_switch(old_thread, next_thread);
}

void maybe_context_switch() {
  disable_interrupt();
  uint32 can_context_switch = 0;
  if (ready_tasks.size > 0) {
    crt_thread->ticks++;
    // Current thread has run out of time slice, switch to next ready thread.
    if (crt_thread->ticks >= crt_thread->priority) {
      crt_thread->ticks = 0;
      can_context_switch = 1;
    }
  }

  if (can_context_switch) {
    do_context_switch();
  }
}

void thread_yield() {
  disable_interrupt();
  if (ready_tasks.size > 0) {
    do_context_switch();
  }
}
