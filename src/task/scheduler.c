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

static pcb_t* main_process;
static thread_node_t* main_thread_node;
static thread_node_t* crt_thread_node;

// processes map
hash_table_t processes_map;
spinlock_t processes_map_lock;

// threads map
hash_table_t threads_map;
spinlock_t threads_map_lock;

// ready task queue
static linked_list_t ready_tasks;

// died task queue
static linked_list_t died_tasks;

static bool main_thread_in_ready_queue = false;

tcb_t* get_crt_thread() {
  return (tcb_t*)(get_crt_thread_node()->ptr);
}

thread_node_t* get_crt_thread_node() {
  return crt_thread_node;
}

pcb_t* get_process(uint32 pid) {
  spinlock_lock(&processes_map_lock);
  pcb_t* process = hash_table_get(&processes_map, pid);
  spinlock_unlock(&processes_map_lock);
  return process;
}

static void destroy_thread(thread_node_t* thread_node);

static void kernel_main_thread(char* argv[]) {
  while (1) {
    // Fast fetch out all nodes from died tasks.
    disable_interrupt();
    linked_list_t died_tasks_copy;
    linked_list_move(&died_tasks_copy, &died_tasks);
    enable_interrupt();

    if (died_tasks_copy.size > 0) {
      // Clean died tasks.
      while (died_tasks_copy.size > 0) {
        thread_node_t* head = died_tasks_copy.head;
        linked_list_remove(&died_tasks_copy, head);
        destroy_thread(head);
      }
      schedule_thread_yield();
    } else {
      cpu_idle();
    }
  }
}

static void ancestor_user_thread() {
  monitor_println("start ancestor user thread ...");

  int32 pid = fork();
  if (pid < 0) {
    monitor_println("fork failed");
  } else if (pid > 0) {
    // parent: thread-2
    monitor_printf("created child process %d\n", pid);
    uint32 status;
    wait(pid, &status);
    monitor_printf("child process exit with code %d\n", status);
  } else {
    // child: thread-3
    printf("child process start ok\n");

    char* prog = "ls";
    printf(">> %s\n", prog);
    char* argv[1];
    argv[0] = "greeting.txt";

    // thread-4
    exec(prog, 1, argv);
  }
}

static void ancestor_kernel_thread(char* argv[]) {
  monitor_println("start ancestor kernel thread ...");

  // thread-2
  pcb_t* crt_process = get_crt_thread()->process;
  tcb_t* thread = create_new_user_thread(crt_process, nullptr, ancestor_user_thread, 0, nullptr);

  // Change this process to user process, so that ancestor_user_thread will switch to user mode.
  // See kernel_thread implementation.
  crt_process->is_kernel_process = false;

  add_thread_to_schedule(thread);
}

void init_scheduler() {
  // Init task queues.
  linked_list_init(&ready_tasks);
  linked_list_init(&died_tasks);

  hash_table_init(&processes_map);
  hash_table_init(&threads_map);

  // Create process 0: kernel main
  main_process = create_process("kernel_main_process", /* is_kernel_process = */true);
  tcb_t* main_thread = create_new_kernel_thread(main_process, nullptr, kernel_main_thread, nullptr);
  main_thread_node = (thread_node_t*)kmalloc(sizeof(thread_node_t));
  main_thread_node->ptr = main_thread;
  crt_thread_node = main_thread_node;
  hash_table_put(&processes_map, main_process->id, main_process);

  // Create process 1: ancestor process
  pcb_t* process = create_process(nullptr, /* is_kernel_process = */true);
  tcb_t* thread = create_new_kernel_thread(process, nullptr, ancestor_kernel_thread, nullptr);
  hash_table_put(&processes_map, process->id, process);
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

// Note: interrupt must be DISABLED before entering this function.
static void do_context_switch() {
  tcb_t* old_thread = get_crt_thread();
  if (old_thread->status == TASK_DIED) {
    // If current thread is dead, add it to died tasks queue and wake up main thread to clean up.
    linked_list_append(&died_tasks, crt_thread_node);
    if (!main_thread_in_ready_queue) {
      linked_list_append(&ready_tasks, main_thread_node);
      main_thread_in_ready_queue = 1;
    }
  }

  thread_node_t* head = ready_tasks.head;
  linked_list_remove(&ready_tasks, head);
  tcb_t* next_thread = (tcb_t*)head->ptr;

  if (old_thread->status == TASK_RUNNING && crt_thread_node != main_thread_node) {
    old_thread->status = TASK_READY;
    head->ptr = (void*)old_thread;
    linked_list_append(&ready_tasks, head);
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

  //monitor_printf("switch to thread %u\n", next_thread->id);
  context_switch(old_thread, next_thread);
}

void maybe_context_switch() {
  disable_interrupt();
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
  disable_interrupt();
  linked_list_append(&ready_tasks, thread_node);
  enable_interrupt();
}

void schedule_thread_yield() {
  disable_interrupt();
  if (ready_tasks.size > 0) {
    do_context_switch();
  } else {
    enable_interrupt();
  }
}

void schedule_thread_exit(int32 exit_code) {
  // Set this thread status as TASK_DIED.
  tcb_t* thread = get_crt_thread();
  thread->exit_code = exit_code;

  disable_interrupt();
  thread->status = TASK_DIED;
  do_context_switch();
}

void schedule_thread_exit_normal() {
  uint32 eax = get_eax();
  exit(eax);
}

static void destroy_thread(thread_node_t* thread_node) {
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  //monitor_printf("clean thread %s\n", thread->name);

  // Remove this thread from its process and release resources.
  pcb_t* process = thread->process;
  remove_process_thread(process, thread);

  // If all threads exit, this process should exit too.
  if (process->threads.size == 0) {
    //monitor_printf("process %d has no threads, destroying\n", process->id);

    // Remove this process from prcoesses map.
    spinlock_lock(&processes_map_lock);
    ASSERT(hash_table_remove(&processes_map, process->id) == process);
    spinlock_unlock(&processes_map_lock);

    // Process exit and destroy.
    //monitor_printf("prcess %d exit with %d\n", process->id, thread->exit_code);
    exit_process(process, thread->exit_code);
    destroy_process(process);
  }

  kfree(thread);
  kfree(thread_node);
}

void add_process_to_schedule(pcb_t* process) {
  spinlock_lock(&processes_map_lock);
  hash_table_put(&processes_map, process->id, process);
  spinlock_unlock(&processes_map_lock);
}
