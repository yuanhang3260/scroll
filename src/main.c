#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "utils/debug.h"
#include "utils/rand.h"
#include "utils/linked_list.h"

char* welcome = "# welcome to scroll kernel #";

int arrayx[10];
double arrayy[10];

void print_shell() {
  monitor_print_with_color("bash", COLOR_GREEN);
  monitor_print(" > ");
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

  linked_list_test();

  monitor_println("idle...");
  while(1) {}
}
