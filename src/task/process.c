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

  process->is_kernel_process = is_kernel_process;

  process->user_thread_stack_indexes = bitmap_create(nullptr, USER_PRCOESS_THREDS_MAX);

  hash_table_init(&process->threads);

  linked_list_init(&process->waiting_thread_nodes);

  hash_table_init(&process->children_exit_codes);

  process->page_dir = clone_crt_page_dir();

  spinlock_init(&process->lock);

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

int32 process_fork() {
  // Create a new process, with page directory cloned from this process.
  pcb_t* process = create_process(nullptr, /* is_kernel_process = */false);
  add_process_to_schedule(process);

  process->parent_pid = get_crt_thread()->process->id;

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
    monitor_printf("faile to get file %s\n", path);
    return -1;
  }

  uint32 size = stat.size;
  char* read_buffer = (char*)kmalloc(size);
  if (read_file(path, read_buffer, 0, size) != size) {
    monitor_printf("faile to read file %s\n", path);
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

  // Copy argv[] to local since we will release all user pages of this process later.
  char** args = copy_str_array(argc, argv);

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
  tcb_t* new_thread = create_new_user_thread(process, path, (void*)exec_entry, argc, args);
  add_thread_to_schedule(new_thread);
  destroy_str_array(argc, args);

  // Exit current thread. This thread will never return to user mode.
  schedule_thread_exit(0);
}

int32 process_wait(uint32 pid, uint32* status) {
  pcb_t* target_process = get_process(pid);
  if (target_process == nullptr) {
    return -1;
  }

  thread_node_t* thread_node = get_crt_thread_node();
  tcb_t* thread = (tcb_t*)thread_node->ptr;
  pcb_t* process = thread->process;

  spinlock_lock(&process->lock);
  hash_table_t* children_exit_codes = &process->children_exit_codes;
  uint32* exit_code = (uint32*)hash_table_get(children_exit_codes, pid);
  spinlock_unlock(&process->lock);
  if (exit_code != nullptr) {
    *status = *exit_code;
    return 0;
  } else {
    thread->status = TASK_WAITING;
    linked_list_append(&target_process->waiting_thread_nodes, thread_node);
    schedule_thread_yield();

    // waken up
    spinlock_lock(&process->lock);
    hash_table_t* children_exit_codes = &process->children_exit_codes;
    uint32* exit_code = (uint32*)hash_table_get(children_exit_codes, pid);
    *status = *exit_code;
    spinlock_unlock(&process->lock);
  }

  return 0;
}

// Process eixts:
//  - wake up threads waiting on this process and set wait result code for them;
void exit_process(pcb_t* process, int32 exit_code) {
  // First, wake up all threads waiting on this process.
  linked_list_t waiting_thread_nodes;
  linked_list_move(&waiting_thread_nodes, &process->waiting_thread_nodes);
  thread_node_t* node = waiting_thread_nodes.head;
  while (node != nullptr) {
    thread_node_t* crt_node = node;
    node = node->next;
    linked_list_remove(&waiting_thread_nodes, crt_node);

    // Set wait result code for waiting tasks (processes).
    tcb_t* wait_thread = (tcb_t*)crt_node->ptr;
    pcb_t* wait_process = wait_thread->process;

    uint32* exit_code_copy = (uint32*)kmalloc(sizeof(uint32));
    *exit_code_copy = exit_code;
    spinlock_lock(&wait_process->lock);
    hash_table_put(&wait_process->children_exit_codes, process->id, exit_code_copy);
    spinlock_unlock(&wait_process->lock);

    // Wake up waiting thread.
    add_thread_node_to_schedule(crt_node);
  }
}

// Destroy a process and release all its resources:
//  - user_thread_stack_indexes bitmap;
//  - children_exit_codes;
//  - all pages (including page dir and page tables);
void destroy_process(pcb_t* process) {
  bitmap_destroy(&process->user_thread_stack_indexes);
  // TODO: release all page frames;

  hash_table_destroy(&process->children_exit_codes);

  kfree(process);
}
