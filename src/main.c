#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "utils/debug.h"
#include "utils/rand.h"

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

void kheap_test() {
  uint32* ptr = (uint32*)kmalloc(32);
  *ptr = 100;
  uint32* ptr1 = (uint32*)kmalloc(200);
  *ptr1 = 101;

  uint32* ptr2 = (uint32*)kmalloc_aligned(4096 * 2);
  *ptr2 = 200;

  kfree(ptr);
  ptr = (uint32*)kmalloc(14);
  *ptr = 100;

  uint8* ptr3 = (uint8*)kmalloc(1);
  *ptr3 = 5;

  uint32* ptr4 = (uint32*)kmalloc_aligned(4096 * 10);
  *ptr4 = 200;

  kfree(ptr);
  kfree(ptr1);
  kfree(ptr2);
  kfree(ptr3);
  kfree(ptr4);

  kheap_validate_print(/* print = */ 1);
}

void kheap_killer() {
  rand_seed(5);
  for (int i = 0; i < 10; i++) {
    uint32 random = rand_range(0, 10);
    monitor_printf("%u ", random);
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
  kheap_killer();

  while(1) {}
}
