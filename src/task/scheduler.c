#include "common/stdio.h"
#include "monitor/monitor.h"
#include "task/thread.h"
#include "task/process.h"
#include "task/scheduler.h"
#include "syscall/syscall.h"
#include "interrupt/interrupt.h"
#include "mem/gdt.h"
#include "mem/kheap.h"
#include "mem/paging.h"
#include "sync/spinlock.h"
#include "sync/mutex.h"
#include "utils/linked_list.h"
#include "utils/hash_table.h"
#include "utils/debug.h"

extern void cpu_idle();
extern int32 get_eax();
extern void context_switch(tcb_t* crt, tcb_t* next);

// ****************************************************************************
static pcb_t* main_process;
static thread_node_t* main_thread_node;
static thread_node_t* crt_thread_node;

// processes map
static hash_table_t processes_map;
static spinlock_t processes_map_lock;

// dead processes waiting for clean
static linked_list_t dead_processes;
static spinlock_t dead_processes_lock;

// threads map
static hash_table_t threads_map;
static spinlock_t threads_map_lock;

// ready task queue
static linked_list_t ready_tasks;
static linked_list_t ready_tasks_candidates;
static spinlock_t ready_tasks_candidates_lock;

// dead task queue
static linked_list_t dead_tasks;

static bool main_thread_in_ready_queue = false;


// ****************************************************************************
tcb_t* get_crt_thread() {
  return (tcb_t*)(get_crt_thread_node()->ptr);
}

thread_node_t* get_crt_thread_node() {
  return crt_thread_node;
}

bool is_kernel_main_thread() {
  return crt_thread_node == main_thread_node;
}

static void destroy_thread(thread_node_t* thread_node);

static void kernel_main_thread(char* argv[]) {
  while (1) {
    // Fast fetch out all nodes from dead tasks.
    disable_interrupt();
    linked_list_t dead_tasks_copy;
    linked_list_move(&dead_tasks_copy, &dead_tasks);
    enable_interrupt();

    if (dead_tasks_copy.size > 0) {
      // Clean dead tasks.
      while (dead_tasks_copy.size > 0) {
        thread_node_t* head = dead_tasks_copy.head;
        linked_list_remove(&dead_tasks_copy, head);
        destroy_thread(head);
      }
      schedule_thread_yield();
    } else {
      cpu_idle();
    }
  }
}

static void init_user_thread() {
  // thread-2
  //monitor_println("start system init process ...");
  exec("init", 0, nullptr);
}

static void init_kernel_thread(char* argv[]) {
  //monitor_println("start init process ...");

  pcb_t* crt_process = get_crt_thread()->process;
  tcb_t* thread = create_new_user_thread(crt_process, nullptr, init_user_thread, 0, nullptr);

  // Change this process to user process, so that ancestor_user_thread will switch to user mode.
  crt_process->is_kernel_process = false;

  add_thread_to_schedule(thread);
}

void init_scheduler() {
  linked_list_init(&ready_tasks);
  linked_list_init(&ready_tasks_candidates);
  spinlock_init(&ready_tasks_candidates_lock);

  linked_list_init(&dead_tasks);

  hash_table_init(&processes_map);
  spinlock_init(&processes_map_lock);

  linked_list_init(&dead_processes);
  spinlock_init(&dead_processes_lock);

  hash_table_init(&threads_map);
  spinlock_init(&threads_map_lock);

  // Create process 0: kernel main process (cpu idle)
  main_process = create_process("kernel_main_process", /* is_kernel_process = */true);
  tcb_t* main_thread = create_new_kernel_thread(main_process, nullptr, kernel_main_thread, nullptr);
  main_thread_node = (thread_node_t*)kmalloc(sizeof(thread_node_t));
  main_thread_node->ptr = main_thread;
  crt_thread_node = main_thread_node;

  // Create process 1: init process
  pcb_t* process = create_process(nullptr, /* is_kernel_process = */true);
  tcb_t* thread = create_new_kernel_thread(process, nullptr, init_kernel_thread, nullptr);
  add_thread_to_schedule(thread);

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

static void process_switch(pcb_t* process) {
  reload_page_directory(&process->page_dir);
}

static void merge_ready_tasks() {
  if (!spinlock_trylock(&ready_tasks_candidates_lock)) {
    return;
  }

  linked_list_concate(&ready_tasks, &ready_tasks_candidates);
  spinlock_unlock(&ready_tasks_candidates_lock);
}

// Note: interrupt must be DISABLED before entering this function.
static void do_context_switch() {
  //monitor_printf("ready_tasks num = %d\n", ready_tasks.size);
  tcb_t* old_thread = get_crt_thread();
  if (old_thread->status == TASK_DEAD) {
    // If current thread is dead, add it to dead tasks queue and wake up main thread to clean up.
    linked_list_append(&dead_tasks, crt_thread_node);
    if (!main_thread_in_ready_queue) {
      linked_list_append(&ready_tasks, main_thread_node);
      main_thread_in_ready_queue = true;
    }
  }

  thread_node_t* head = ready_tasks.head;
  linked_list_remove(&ready_tasks, head);
  tcb_t* next_thread = (tcb_t*)head->ptr;

  if (old_thread->status == TASK_RUNNING && crt_thread_node != main_thread_node) {
    old_thread->status = TASK_READY;
    linked_list_append(&ready_tasks, crt_thread_node);
  }

  next_thread->status = TASK_RUNNING;
  crt_thread_node = head;
  if (head == main_thread_node) {
    main_thread_in_ready_queue = 0;
  }

  // Setup env for next thread (and maybe a different process)
  update_tss_esp((uint32)next_thread + KERNEL_STACK_SIZE);
  if (old_thread->process != next_thread->process) {
    process_switch(next_thread->process);
  }

  //monitor_printf("thread %u switch to thread %u\n", old_thread->id, next_thread->id);
  context_switch(old_thread, next_thread);
}

void maybe_context_switch() {
  disable_interrupt();
  merge_ready_tasks();

  uint32 need_context_switch = 0;
  if (ready_tasks.size > 0) {
    tcb_t* crt_thread = get_crt_thread();
    crt_thread->ticks++;
    // Current thread has run out of time slice, switch to next ready thread.
    if (crt_thread->ticks >= crt_thread->priority) {
      crt_thread->ticks = 0;
      need_context_switch = 1;
    }
  }

  if (need_context_switch) {
    //monitor_printf("context_switch yes, %d ready tasks\n", ready_tasks.size);
    do_context_switch();
  } else {
    //monitor_println("context_switch no");
    enable_interrupt();
  }
}

void add_thread_to_schedule(tcb_t* thread) {
  thread_node_t* node = (thread_node_t*)kmalloc(sizeof(thread_node_t));
  node->ptr = (void*)thread;
  add_thread_node_to_schedule(node);
}

void add_thread_node_to_schedule(thread_node_t* thread_node) {
  spinlock_lock(&ready_tasks_candidates_lock);
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  thread->status = TASK_READY;
  linked_list_append(&ready_tasks_candidates, thread_node);
  spinlock_unlock(&ready_tasks_candidates_lock);
}

void add_thread_node_to_schedule_head(thread_node_t* thread_node) {
  spinlock_lock(&ready_tasks_candidates_lock);
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  thread->status = TASK_READY;
  linked_list_insert_to_head(&ready_tasks_candidates, thread_node);
  spinlock_unlock(&ready_tasks_candidates_lock);
}

void schedule_thread_yield() {
  disable_interrupt();
  merge_ready_tasks();

  //monitor_printf("thread %d yield\n", get_crt_thread()->id);

  disable_interrupt();
  merge_ready_tasks();

  if (ready_tasks.size == 0 && crt_thread_node != main_thread_node) {
    linked_list_append(&ready_tasks, main_thread_node);
    main_thread_in_ready_queue = 1;
  }

  if (ready_tasks.size > 0) {
    do_context_switch();
  }
}

void schedule_mark_thread_block() {
  tcb_t* thread = (tcb_t*)crt_thread_node->ptr;
  thread->status = TASK_WAITING;
}

void schedule_thread_exit() {
  // Detach this thread from its process.
  tcb_t* thread = get_crt_thread();
  pcb_t* process = thread->process;
  remove_process_thread(process, thread);

  // Mark this thread TASK_dead.
  disable_interrupt();
  merge_ready_tasks();
  thread->status = TASK_DEAD;
  do_context_switch();
}

void schedule_thread_exit_normal() {
  //tcb_t* thread = get_crt_thread();
  //pcb_t* process = thread->process;
  //monitor_printf("process %d thread %d exit\n", process->id, thread->id);
  thread_exit();
}

// TODO: move it to thread.c
static void destroy_thread(thread_node_t* thread_node) {
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  //monitor_printf("clean thread %s\n", thread->name);

  // // If all threads exit, this process should exit too.
  // if (process->threads.size == 0) {
  //   //monitor_printf("process %d has no threads, destroying\n", process->id);

  //   // Remove this process from prcoesses map.
  //   spinlock_lock(&processes_map_lock);
  //   ASSERT(hash_table_remove(&processes_map, process->id) == process);
  //   spinlock_unlock(&processes_map_lock);

  //   // Process exit and destroy.
  //   //monitor_printf("process %d exit with %d\n", process->id, thread->exit_code);
  //   process_exit();
  //   destroy_process(process);
  // }

  kfree(thread);
  kfree(thread_node);
}

void add_new_process(pcb_t* process) {
  spinlock_lock(&processes_map_lock);
  hash_table_put(&processes_map, process->id, process);
  spinlock_unlock(&processes_map_lock);
}

void add_dead_process(pcb_t* process) {
  spinlock_lock(&dead_processes_lock);
  linked_list_append_ele(&dead_processes, process);
  spinlock_unlock(&dead_processes_lock);
}
