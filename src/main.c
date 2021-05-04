#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "utils/debug.h"

char* welcome = "# welcome to scroll kernel #";

int arrayx[10];
double arrayy[10];

void print_shell() {
  monitor_print_with_color("bash", COLOR_GREEN);
  monitor_print(" > ");
}

void memory_killer() {
  uint32 *ptr = (uint32*)KERNEL_PLACEMENT_ADDR_START;
  while(1) {
    *ptr = 3;
    ptr += 1;
  }
}

void kheap_killer() {
  uint32* ptr = (uint64*)kmalloc(32);
  *ptr = 100;
  monitor_printf("ptr = %x, *ptr = %d\n", ptr, *ptr);
}

int main() {
  monitor_clear();
  monitor_println(welcome);
  //print_shell();

  init_idt();
  // enable_interrupt();
  // init_timer(TIMER_FREQUENCY);

  init_paging();
  //memory_killer();

  init_kheap();
  kheap_killer();

  while(1) {}
}
