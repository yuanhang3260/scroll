#include "common/common.h"
#include "common/stdlib.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "task/thread.h"
#include "utils/debug.h"
#include "utils/rand.h"
#include "utils/linked_list.h"

char* welcome = "# welcome to scroll kernel #";

void k_thread_a(void*);

void print_shell() {
  monitor_print_with_color("bash", COLOR_GREEN);
  monitor_print(" > ");
}

void k_thread_a(void* argv) {
  while(1) {
    monitor_println("thread_1");
  }
}

int main() {
  monitor_clear();
  monitor_println(welcome);

  init_idt();
  enable_interrupt();
  init_timer(TIMER_FREQUENCY);

  init_paging();
  //memory_killer();

  init_kheap();
  //kheap_test();
  //kheap_killer();

  thread_start("k_thread_a", 31, k_thread_a, 0);

  monitor_println("idle...");
  while(1) {}
}
