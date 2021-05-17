#include "common/common.h"
#include "common/stdlib.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"
#include "mem/gdt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "task/thread.h"
#include "task/process.h"
#include "task/scheduler.h"
#include "syscall/syscall.h"
#include "utils/debug.h"
#include "utils/rand.h"
#include "utils/linked_list.h"

char* welcome = "# welcome to scroll kernel #";

int main() {
  init_gdt();

  monitor_clear();
  monitor_println(welcome);

  init_idt();
  enable_interrupt();
  init_timer(TIMER_FREQUENCY);

  init_paging();
  init_kheap();
  //kheap_test();
  //kheap_killer();

  init_scheduler();

  start_scheduler();

  // Never should reach here.
  PANIC();
}
