#include "common/stdlib.h"
#include "task/process.h"
#include "task/thread.h"
#include "task/scheduler.h"
#include "monitor/monitor.h"
#include "mem/kheap.h"
#include "mem/paging.h"
#include "fs/file.h"
#include "fs/fs.h"
#include "elf/elf.h"
#include "utils/string.h"
#include "utils/debug.h"
#include "utils/hash_table.h"

static uint32 next_pid = 0;

// TODO: remove it
extern page_directory_t* current_page_directory;

// ****************************************************************************
pcb_t* create_process(char* name, uint8 is_kernel_process) {
  pcb_t* process = (pcb_t*)kmalloc(sizeof(pcb_t));
  memset(process, 0, sizeof(pcb_t));

  process->id = next_pid++;
  if (name != nullptr) {
    strcpy(process->name, name);
  } else {
    char buf[32];
    sprintf(buf, "process-%u", process->id);
    strcpy(process->name, buf);
  }

  process->parent_pid = 0;

  process->status = PROCESS_NORMAL;

  hash_table_init(&process->threads);

  process->user_thread_stack_indexes = bitmap_create(nullptr, USER_PRCOESS_THREDS_MAX);

  process->is_kernel_process = is_kernel_process;

  process->exit_code = 0;

  hash_table_init(&process->children_processes);

  hash_table_init(&process->exit_children_processes);

  process->waiting_parent = nullptr;

  process->page_dir = clone_crt_page_dir();

  spinlock_init(&process->lock);

  add_new_process(process);
  return process;
}

tcb_t* create_new_kernel_thread(pcb_t* process, char* name, void* function, char** argv) {
  tcb_t* thread = init_thread(nullptr, name, function, 0, argv, false, THREAD_DEFAULT_PRIORITY);
  add_process_thread(process, thread);
  return thread;
}

tcb_t* create_new_user_thread(
    pcb_t* process, char* name, void* user_function, uint32 argc, char** argv) {
  // Create new thread on this process.
  tcb_t* thread = init_thread(
      nullptr, name, user_function, argc, argv, true, THREAD_DEFAULT_PRIORITY);
  add_process_thread(process, thread);

  // Allocate a user space stack for this thread.
  uint32 stack_index;
  spinlock_lock(&process->lock);
  if (!bitmap_allocate_first_free(&process->user_thread_stack_indexes, &stack_index)) {
    spinlock_unlock(&process->lock);
    return nullptr;
  }
  spinlock_unlock(&process->lock);

  thread->user_stack_index = stack_index;
  uint32 thread_stack_top = USER_STACK_TOP - stack_index * USER_STACK_SIZE;
  map_page((uint32)thread_stack_top - PAGE_SIZE);

  prepare_user_stack(thread, thread_stack_top, argc, argv, (uint32)schedule_thread_exit_normal);

  return thread;
}

void add_process_thread(pcb_t* process, tcb_t* thread) {
  thread->process = process;
  spinlock_lock(&process->lock);
  hash_table_put(&process->threads, thread->id, thread);
  spinlock_unlock(&process->lock);
}

void remove_process_thread(pcb_t* process, tcb_t* thread) {
  spinlock_lock(&process->lock);
  tcb_t* removed_thread = hash_table_remove(&process->threads, thread->id);
  ASSERT(removed_thread == thread);
  if (thread->user_stack_index >= 0) {
    //monitor_printf("thread %d release user stack %d\n", thread->id, thread->user_stack_index);
    bitmap_clear_bit(&process->user_thread_stack_indexes, thread->user_stack_index);
  }
  spinlock_unlock(&process->lock);
}

void add_child_process(pcb_t* parent, pcb_t* child) {
  spinlock_lock(&parent->lock);
  hash_table_put(&parent->children_processes, child->id, child);
  spinlock_unlock(&parent->lock);
}

int32 process_fork() {
  // Create a new process, with page directory cloned from this process.
  pcb_t* process = create_process(nullptr, /* is_kernel_process = */false);
  add_new_process(process);

  pcb_t* parent_process = get_crt_thread()->process;
  process->parent_pid = parent_process->id;
  add_child_process(parent_process, process);

  // Copy current thread and prepare for its kernel and user stacks.
  tcb_t* thread = fork_crt_thread();
  thread->syscall_ret = true;

  // Add new thread to new process.
  add_process_thread(process, thread);

  // The user stack of this thread should be marked in process.
  bitmap_set_bit(&process->user_thread_stack_indexes, thread->user_stack_index);

  add_thread_to_schedule(thread);

  // Parent should return the new pid.
  return process->id;
}

int32 process_exec(char* path, uint32 argc, char* argv[]) {
  // TODO: disallow exec if there are multiple threads running on this process?

  // Read elf binary file.
  file_stat_t stat;
  if (stat_file(path, &stat) != 0) {
    monitor_printf("Command %s not found\n", path);
    return -1;
  }

  uint32 size = stat.size;
  char* read_buffer = (char*)kmalloc(size);
  if (read_file(path, read_buffer, 0, size) != size) {
    monitor_printf("Failed to load cmd %s\n", path);
    kfree(read_buffer);
    return -1;
  }

  // Destroy all threads of this process (except current thread).
  tcb_t* crt_thread = get_crt_thread();
  pcb_t* process = crt_thread->process;
  hash_table_t* threads = &process->threads;
  tcb_t* keep_thread = hash_table_remove(threads, crt_thread->id);
  ASSERT(keep_thread == crt_thread);
  hash_table_destroy(threads);

  // TODO: remove destroyed threads from schedule task queues.

  spinlock_lock(&process->lock);
  hash_table_init(threads);
  hash_table_put(threads, keep_thread->id, crt_thread);
  // Release all user stacks.
  bitmap_clear(&process->user_thread_stack_indexes);
  spinlock_unlock(&process->lock);

  // Copy path and argv[] to local since we will release all user pages of this process later.
  char** args = copy_str_array(argc, argv);
  char* path_copy = (char*)kmalloc(strlen(path) + 1);
  strcpy(path_copy, path);


  // Release all user space pages of this process.
  // User virtual space is 4MB - 3G, totally 1024 * 3/4 - 1 = 767 page dir entries.
  release_pages(4 * 1024 * 1024, 767 * 1024);

  // Load elf binary.
  uint32 exec_entry;
  if (load_elf(read_buffer, &exec_entry)) {
    monitor_printf("faile to load elf file %s\n", path);
    return -1;
  }
  //monitor_printf("entry = %x\n", exec_entry);
  kfree(read_buffer);

  // Create a new thread to exec new program.
  tcb_t* new_thread = create_new_user_thread(process, path_copy, (void*)exec_entry, argc, args);
  add_thread_to_schedule(new_thread);
  destroy_str_array(argc, args);
  kfree(path_copy);

  // Exit current thread. This thread will never return to user mode.
  schedule_thread_exit();
}

// Process wait
int32 process_wait(uint32 pid, uint32* status) {
  thread_node_t* thread_node = get_crt_thread_node();
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  pcb_t* process = thread->process;

  pcb_t* child = hash_table_get(&process->children_processes, pid);
  ASSERT(child != nullptr);
  spinlock_lock(&child->lock);
  while (1) {
    if (child->status == PROCESS_EXIT || child->status == PROCESS_EXIT_ZOMBIE) {
      break;
    }
    child->waiting_parent = thread_node;
    spinlock_unlock(&child->lock);
    schedule_thread_block();

    spinlock_lock(&child->lock);
  }
  // Reap child exit code and release pcb struct.
  if (status != nullptr) {
    *status = child->exit_code;
  }
  spinlock_unlock(&child->lock);

  // Add child process to dead list
  add_dead_process(child);

  return 0;
}

// Process eixts:
//  - All remaining children processes should be adopted by init process;
//  - If parent is waiting, wake it up and set exit status;
//  - release all resources (except pcb);
void process_exit(int32 exit_code) {
  thread_node_t* thread_node = get_crt_thread_node();
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  pcb_t* process = thread->process;

  // Set process exit info, and wake up waiting parent if exists.
  spinlock_lock(&process->lock);
  if (process->threads.size > 1) {
    // If multi threads are running on this process, disallow exit().
    spinlock_unlock(&process->lock);
    return;
  }

  process->exit_code = exit_code;
  process->status = PROCESS_EXIT;
  if (process->waiting_parent != nullptr) {
    // Notify parent.
    add_thread_node_to_schedule(process->waiting_parent);
  } else {
    //process->status = PROCESS_EXIT_ZOMBIE;
  }
  spinlock_unlock(&process->lock);

  schedule_thread_exit();
}

// Destroy a process and release all its resources:
//  - user_thread_stack_indexes bitmap;
//  - all pages (including page dir and page tables);
void destroy_process(pcb_t* process) {
  bitmap_destroy(&process->user_thread_stack_indexes);
  // TODO: release all page frames;

  kfree(process);
}
