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
#include "utils/debug.h"
#include "utils/rand.h"
#include "utils/linked_list.h"

char* welcome = "# welcome to scroll kernel #";

void test_thread(int argc, char* argv[]) {
  monitor_printf("argc = %d\n", argc);
  for (int i = 0; i < argc; i++) {
    monitor_printf("arg: %s\n", argv[i]);
  }
  while(1) {
    monitor_println(argv[2]);
  }
}

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

  pcb_t* process = create_process(nullptr, /* is_kernel_process = */false);

  char* argv[2];
  argv[0] = "hello";
  argv[1] = "scroll";

  tcb_t* thread = create_new_user_thread(process, nullptr, test_thread, 2, argv);
  add_thread_to_schedule(thread);

  char* argv2[2];
  argv2[0] = "hello";
  argv2[1] = "world";
  tcb_t* thread2 = create_new_user_thread(process, nullptr, test_thread, 2, argv2);
  add_thread_to_schedule(thread2);

  start_scheduler();

  // Never should reach here.
  PANIC();
}
