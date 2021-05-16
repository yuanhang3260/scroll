#include "monitor/monitor.h"
#include "task/thread.h"
#include "task/process.h"
#include "task/scheduler.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "mem/gdt.h"
#include "common/stdlib.h"

extern uint32 get_esp();

static uint32 next_thread_id = 0;

static void kernel_thread(thread_func* function, char** func_arg, tcb_t* thread) {
  if (thread->process->is_kernel_process) {
    // Kernel thread.
    function(func_arg);
    schedule_thread_exit(thread);
  } else {
    // User thread - switch to user space.
    uint32 interrupt_stack = (uint32)thread->self_kstack + sizeof(switch_stack_t);
    asm volatile ("movl %0, %%esp; jmp interrupt_exit" : : "g" (interrupt_stack) : "memory");
  }
}

tcb_t* init_thread(char* name, thread_func function, uint32 argc, char** argv,
    uint8 user_thread, uint32 priority) {
  // Allocate one page as tcb_t and kernel stack for each thread.
  tcb_t* thread = (tcb_t*)kmalloc_aligned(PAGE_SIZE);
  map_page((uint32)thread);
  memset(thread, 0, sizeof(tcb_t));

  thread->id = next_thread_id++;
  if (name != 0) {
    strcpy(thread->name, name);
  } else {
    char buf[32];
    sprintf(buf, "thread-%u", thread->id);
    strcpy(thread->name, buf);
  }

  thread->status = TASK_READY;
  thread->ticks = 0;
  thread->priority = priority;
  thread->stack_magic = THREAD_STACK_MAGIC;

  // Init thread stack.
  thread->self_kstack =
      (void*)thread + PAGE_SIZE - (sizeof(interrupt_stack_t) + sizeof(switch_stack_t));
  switch_stack_t* kthread_stack = (switch_stack_t*)thread->self_kstack;

  kthread_stack->edi = 0;
  kthread_stack->esi = 0;
  kthread_stack->ebp = 0;
  kthread_stack->ebx = 0;
  kthread_stack->edx = 0;
  kthread_stack->ecx = 0;
  kthread_stack->eax = 0;

  kthread_stack->eip = kernel_thread;
  kthread_stack->function = function;
  kthread_stack->argv = argv;
  kthread_stack->tcb = thread;

  // For user thread, there are some more steps to do:
  //  - Prepare the kernel interrupt stack context for later to switch to user mode.
  //  - Allocate a user space stack;
  //  - Prepare the user stack:
  //    - 1. Copy args to stack;
  //    - 2. Set the return address as thread_exit syscall so that it can exit normally;
  if (user_thread) {
    interrupt_stack_t* interrupt_stack =
        (interrupt_stack_t*)((uint32)thread->self_kstack + sizeof(switch_stack_t));

    // data segemnts
    interrupt_stack->ds = SELECTOR_U_DATA;

    // general regs
    interrupt_stack->edi = 0;
    interrupt_stack->esi = 0;
    interrupt_stack->ebp = 0;
    interrupt_stack->esp = 0;
    interrupt_stack->ebx = 0;
    interrupt_stack->edx = 0;
    interrupt_stack->ecx = 0;
    interrupt_stack->eax = 0;

    // user-level code env
    interrupt_stack->eip = (uint32)function;
    interrupt_stack->cs = SELECTOR_U_CODE;
    interrupt_stack->eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;

    // user stack
    interrupt_stack->user_ss = SELECTOR_U_DATA;
  }

  return thread;
}

// Copy args to user stack and set return address.
// Return the stack top for thread function to run.
uint32 prepare_user_stack(
    tcb_t* thread, uint32 stack_top, uint32 argc, char** argv, uint32 return_addr) {
  uint32 total_argv_length = 0;
  // Reserve space to copy the argv strings.
  for (int i = 0; i < argc; i++) {
    char c;
    int j = 0;
    while ((c != argv[i][j++]) != '\0') {
      total_argv_length++;
    }
    total_argv_length++;
  }
  stack_top -= total_argv_length;
  stack_top = stack_top / 4 * 4;

  char* args[argc + 1];
  args[0] = thread->name;

  char* argv_chars_addr = (char*)stack_top;
  for (int i = 0; i < argc; i++) {
    uint32 length = strcpy(argv_chars_addr, argv[i]);
    args[i + 1] = (char*)argv_chars_addr;
    argv_chars_addr += (length + 1);
  }

  // Copy args[] array to stack.
  stack_top -= ((argc + 1) * 4);
  uint32 argv_start = stack_top;;
  for (int i = 0; i < argc + 1; i++) {
    *((char**)(argv_start + i * 4)) = args[i];
  }

  stack_top -= 4;
  *((uint32*)stack_top) = argv_start;
  stack_top -= 4;
  *((uint32*)stack_top) = argc + 1;

  // Set thread return address.
  stack_top -= 4;
  *((uint32*)stack_top) = return_addr;

  interrupt_stack_t* interrupt_stack =
      (interrupt_stack_t*)((uint32)thread->self_kstack + sizeof(switch_stack_t));
  interrupt_stack->user_esp = stack_top;
  return stack_top;
}
