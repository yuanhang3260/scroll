#include "common/stdlib.h"
#include "monitor/monitor.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "task/thread.h"
#include "task/process.h"
#include "task/scheduler.h"
#include "syscall/syscall_impl.h"
#include "utils/debug.h"

static int32 syscall_exit_impl(int32 exit_code) {
  schedule_thread_exit(exit_code);
}

static int32 syscall_fork_impl() {
  return process_fork();
}

static int32 syscall_exec_impl(char* path, uint32 argc, char* argv[]) {
  //monitor_printf("path = %s, argnum = %d, arg1 = %s, arg2 = %s", path, argc, argv[0], argv[1]);
}

static int32 syscall_yield_impl() {

}

int32 syscall_handler(isr_params_t isr_params) {
  // syscall num saved in eax.
  // args list: ecx, edx, ebx, esi, edi
  uint32 syscall_num = isr_params.eax;

  switch (syscall_num) {
    case SYSCALL_EXIT_NUM:
      return syscall_exit_impl((int32)isr_params.ecx);
    case SYSCALL_FORK_NUM:
      return syscall_fork_impl();
    case SYSCALL_EXEC_NUM:
      return syscall_exec_impl((char*)isr_params.ecx, isr_params.edx, (char**)isr_params.ebx);
    case SYSCALL_YIELD_NUM:
      return syscall_yield_impl();
    default:
      PANIC();
  }
}
