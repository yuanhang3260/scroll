#include "monitor/monitor.h"
#include "task/thread.h"
#include "task/scheduler.h"
#include "interrupt/interrupt.h"
#include "mem/kheap.h"
#include "utils/linked_list.h"
#include "utils/debug.h"

extern void cpu_idle();
extern void context_switch(tcb_t* crt, tcb_t* next);

static tcb_t* main_thread;
static tcb_t* crt_thread;

static linked_list_t ready_tasks;
static linked_list_t blocking_tasks;
static linked_list_t died_tasks;

static uint32 main_thread_in_ready_queue = 0;

static void kernel_main_thread() {
  while (1) {
    // Clean died tasks.
    while (died_tasks.size > 0) {
      linked_list_node_t* head = died_tasks.head;
      linked_list_remove(&died_tasks, head);
      tcb_t* thread = (tcb_t*)head->ptr;
      //monitor_printf("clean thread %s\n", thread->name);
      kfree(head);
      kfree(thread);
    }

    cpu_idle();
  }
}

static void thread_start(tcb_t* thread) {
  asm volatile (
   "movl %0, %%esp; \
    pop %%edi; \
    pop %%esi; \
    pop %%ebp; \
    pop %%ebx; \
    pop %%edx; \
    pop %%ecx; \
    pop %%eax; \
    ret": : "g" (thread->self_kstack) : "memory");
}

void init_scheduler() {
  // Init task queues.
  ready_tasks = create_linked_list();
  blocking_tasks = create_linked_list();

  // Create kernel main thread.
  main_thread = init_thread("kernel_main_thread", kernel_main_thread, (void*)0, 10);
  crt_thread = main_thread;
}

void start_scheduler() {
  thread_start(main_thread);
  // Never should reach here!
  PANIC();
}

tcb_t* create_thread(char* name, thread_func function, void* func_arg, uint32 priority) {
  if (priority == 0) {
    priority = 10;
  }
  tcb_t* thread = init_thread(name, function, func_arg, priority);
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
  tcb_t* old_thread = crt_thread;
  tcb_t* next_thread = (tcb_t*)head->ptr;

  if (old_thread->status == TASK_RUNNING && old_thread != main_thread) {
    old_thread->status = TASK_READY;
    head->ptr = (void*)old_thread;
    linked_list_append(&ready_tasks, head);
  }

  next_thread->status = TASK_RUNNING;
  crt_thread = next_thread;
  if (next_thread == main_thread) {
    main_thread_in_ready_queue = 0;
  }
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
    //monitor_println("context_switch yes");
    do_context_switch();
  } else {
    //monitor_println("context_switch no");
    enable_interrupt();
  }
}

void thread_yield() {
  disable_interrupt();
  if (ready_tasks.size > 0) {
    do_context_switch();
  } else {
    enable_interrupt();
  }
}

// Put this thread to died_tasks queue, and wake up main thread to 
void thread_destroy(tcb_t* thread) {
  thread->status = TASK_DIED;
  linked_list_node_t* node = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  node->ptr = (void*)thread;
  disable_interrupt();
  linked_list_append(&died_tasks, node);

  // If any of these conditions matches, wake up the main thread to clean died tasks.
  //  - If died tasks queue length is over threshold;
  //  - If there is no other ready tasks to execute;
  if (!main_thread_in_ready_queue && (died_tasks.size >= 1 || ready_tasks.size == 0)) {
    linked_list_node_t* node = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
    node->ptr = (void*)main_thread;
    linked_list_append(&ready_tasks, node);
    main_thread_in_ready_queue = 1;
  }

  do_context_switch();
}
