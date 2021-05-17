#include "monitor/monitor.h"
#include "task/thread.h"
#include "task/process.h"
#include "task/scheduler.h"
#include "syscall/syscall.h"
#include "interrupt/interrupt.h"
#include "mem/gdt.h"
#include "mem/kheap.h"
#include "mem/paging.h"
#include "utils/linked_list.h"
#include "utils/debug.h"

extern void cpu_idle();
extern void context_switch(tcb_t* crt, tcb_t* next);

static pcb_t* main_process;
static tcb_t* main_thread;
static tcb_t* crt_thread;

static linked_list_t ready_tasks;
static linked_list_t blocking_tasks;
static linked_list_t died_tasks;

static bool main_thread_in_ready_queue = 0;

static void kernel_main_thread(char* argv[]) {
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

static void ancestor_user_thread() {
  monitor_println("start ancestor user thread ...");
  while (1) {}
}

static void ancestor_kernel_thread(char* argv[]) {
  monitor_println("start ancestor kernel thread ...");
  
  pcb_t* crt_process = crt_thread->process;
  tcb_t* thread = create_new_user_thread(crt_process, nullptr, ancestor_user_thread, 0, nullptr);

  // Change this process to user process, so that ancestor_user_thread will switch to user mode.
  // See kernel_thread implementation.
  crt_process->is_kernel_process = false;

  add_thread_to_schedule(thread);
}

void init_scheduler() {
  // Init task queues.
  ready_tasks = create_linked_list();
  blocking_tasks = create_linked_list();

  // Create process 0: kernel main
  main_process = create_process("kernel_main_process", /* is_kernel_process = */true);
  main_thread = create_new_kernel_thread(main_process, nullptr, kernel_main_thread, nullptr);
  crt_thread = main_thread;

  // Create process 1: ancestor user process
  pcb_t* process = create_process(nullptr, /* is_kernel_process = */true);
  tcb_t* thread = create_new_kernel_thread(process, nullptr, ancestor_kernel_thread, nullptr);
  add_thread_to_schedule(thread);
}

void start_scheduler() {
  // Start the main thread.
  asm volatile (
   "movl %0, %%esp; \
    pop %%edi; \
    pop %%esi; \
    pop %%ebp; \
    pop %%ebx; \
    pop %%edx; \
    pop %%ecx; \
    pop %%eax; \
    ret": : "g" (main_thread->self_kstack) : "memory");

  // Never should reach here!
  PANIC();
}

void add_thread_to_schedule(tcb_t* thread) {
  linked_list_node_t* node = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  node->ptr = (void*)thread;
  disable_interrupt();
  linked_list_append(&ready_tasks, node);
  enable_interrupt();
}

static void process_switch(pcb_t* process) {
  reload_page_directory(&process->page_dir);
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

  // Setup env for next thread (and maybe a different process)
  update_tss_esp((uint32)next_thread + PAGE_SIZE);
  if (old_thread->process != next_thread->process) {
    process_switch(next_thread->process);
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

void schedule_thread_yield() {
  disable_interrupt();
  if (ready_tasks.size > 0) {
    do_context_switch();
  } else {
    enable_interrupt();
  }
}

// Put this thread to died_tasks queue, and maybe wake up main thread to clean.
void schedule_thread_exit(int32 exit_code) {
  tcb_t* thread = crt_thread;
  // Remove this thread from its process and release resources.
  disable_interrupt();
  pcb_t* process = thread->process;
  linked_list_remove_ele(&process->threads, thread);
  if (thread->user_stack_index >= 0) {
    //monitor_printf("thread %d release user stack %d\n", thread->id, thread->user_stack_index);
    bitmap_clear_bit(&process->user_thread_stack_indexes, thread->user_stack_index);
  }
  enable_interrupt();

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

void schedule_thread_exit_normal() {
  exit(0);
}
