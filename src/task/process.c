#include "common/stdlib.h"
#include "task/process.h"
#include "task/thread.h"
#include "task/scheduler.h"
#include "monitor/monitor.h"
#include "mem/kheap.h"
#include "mem/paging.h"
#include "utils/string.h"

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

  process->is_kernel_process = is_kernel_process;

  process->user_thread_stack_indexes = bitmap_create(nullptr, USER_PRCOESS_THREDS_MAX);

  process->page_dir = clone_crt_page_dir();

  return process;
}

tcb_t* create_new_kernel_thread(pcb_t* process, char* name, void* function, char** argv) {
  tcb_t* thread = init_thread(nullptr, name, function, 0, argv, false, THREAD_DEFAULT_PRIORITY);
  thread->process = process;
  linked_list_append_ele(&process->threads, thread);
  return thread;
}

tcb_t* create_new_user_thread(
    pcb_t* process, char* name, void* user_function, uint32 argc, char** argv) {
  // Create new thread on this process.
  tcb_t* thread = init_thread(
      nullptr, name, user_function, argc, argv, true, THREAD_DEFAULT_PRIORITY);
  thread->process = process;
  linked_list_append_ele(&process->threads, thread);

  // Allocate a user space stack for this thread.
  uint32 stack_index;
  if (!bitmap_allocate_first_free(&process->user_thread_stack_indexes, &stack_index)) {
    return 0;
  }
  thread->user_stack_index = stack_index;
  uint32 thread_stack_top = USER_STACK_TOP - stack_index * USER_STACK_SIZE;
  map_page((uint32)thread_stack_top - PAGE_SIZE);

  prepare_user_stack(thread, thread_stack_top, argc, argv, (uint32)schedule_thread_exit_normal);

  return thread;
}

int32 process_fork() {
  // Create a new process, with page directory cloned from this process.
  pcb_t* process = create_process(nullptr, /* is_kernel_process = */false);

  // Copy current thread and prepare for its kernel and user stacks.
  tcb_t* thread = fork_crt_thread();
  thread->syscall_ret = true;

  // Add new thread to new process.
  linked_list_append_ele(&process->threads, thread);
  thread->process = process;
  add_thread_to_schedule(thread);

  // The user stack of this thread should be marked in process.
  bitmap_set_bit(&process->user_thread_stack_indexes, thread->user_stack_index);

  // Parent should return the new pid.
  return process->id;
}

int32 process_exec(char* path, uint32 argc, char* argv[]) {
  // TODO: read elf bin from path and load it.
  void* exec_entry = (void*)path;

  // Destroy all threads of this process (except current thread).
  tcb_t* thread = get_crt_thread();
  pcb_t* process = thread->process;
  linked_list_t* threads = &process->threads;
  linked_list_node_t* keep;
  while (process->threads.size > 0) {
    linked_list_node_t* head = threads->head;
    linked_list_remove(threads, head);
    if (head->ptr == thread) {
      keep = head;
      continue;
    }
    kfree(head);
    kfree(head->ptr);
  }

  linked_list_append(threads, keep);

  // Release all user stacks.
  bitmap_clear(&process->user_thread_stack_indexes);

  // Copy argv[] to local since we will release all user pages of this process later.
  char** args = copy_str_array(argc, argv);

  // Release all user space pages of this process.
  // User virtual space is 4MB - 3G, totally 1024 * 3/4 - 1 = 767 page dir entries.
  release_pages(4 * 1024 * 1024, 767 * 1024);

  // Create a new thread to exec new program.
  tcb_t* new_thread = create_new_user_thread(process, nullptr, exec_entry, argc, args);
  add_thread_to_schedule(new_thread);
  destroy_str_array(argc, args);

  // Exit current thread. This thread will never return to user mode.
  schedule_thread_exit(0);
}
