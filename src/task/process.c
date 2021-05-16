#include "common/stdlib.h"
#include "task/process.h"
#include "task/thread.h"
#include "task/scheduler.h"
#include "monitor/monitor.h"
#include "mem/kheap.h"
#include "mem/paging.h"

static uint32 next_pid = 1;

// TODO: remove it
extern page_directory_t* current_page_directory;

// ****************************************************************************
pcb_t* create_process(char* name, uint8 is_kernel_process) {
  pcb_t* process = (pcb_t*)kmalloc(sizeof(pcb_t));
  memset(process, 0, sizeof(pcb_t));

  process->id = next_pid++;
  if (name != 0) {
    strcpy(process->name, name);
  } else {
    char buf[32];
    sprintf(buf, "process-%u", process->id);
    strcpy(process->name, buf);
  }

  process->is_kernel_process = is_kernel_process;

  process->user_thread_stack_indexes = bitmap_create(0, USER_PRCOESS_THREDS_MAX);

  // TODO: clone current page directory;
  //process->page_dir = clone_crt_page_dir();
  process->page_dir = (uint32)current_page_directory->page_dir_entries_phy;

  return process;
}

tcb_t* create_new_kernel_thread(pcb_t* process, char* name, void* function, char** argv) {
  tcb_t* thread = init_thread(name, function, 0, argv, false, THREAD_DEFAULT_PRIORITY);
  thread->process = process;
  linked_list_append_ele(&process->threads, thread);
  return thread;
}

tcb_t* create_new_user_thread(
    pcb_t* process, char* name, void* user_function, uint32 argc, char** argv) {
  // Create new thread on this process.
  tcb_t* thread = init_thread(name, user_function, argc, argv, true, THREAD_DEFAULT_PRIORITY);
  thread->process = process;
  linked_list_append_ele(&process->threads, thread);

  // Allocate a user space stack for this thread.
  uint32 stack_index;
  if (!bitmap_allocate_first_free(&process->user_thread_stack_indexes, &stack_index)) {
    return 0;
  }
  uint32 thread_stack_top = USER_STACK_TOP - stack_index * USER_STACK_SIZE;
  map_page((uint32)thread_stack_top - PAGE_SIZE);
  prepare_user_stack(thread, thread_stack_top, argc, argv, (uint32)schedule_thread_exit);

  return thread;
}
