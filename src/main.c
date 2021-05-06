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
  uint8* ptr = (uint8*)kmalloc(32);
  *ptr = 100;
  uint8* ptr1 = (uint8*)kmalloc(200);
  *ptr1 = 101;

  uint8* ptr2 = (uint8*)kmalloc_aligned(4096 * 2);
  *ptr2 = 200;

  kfree(ptr);
  ptr = (uint8*)kmalloc(14);
  *ptr = 100;

  uint8* ptr3 = (uint8*)kmalloc(1);
  *ptr3 = 5;

  uint8* ptr4 = (uint8*)kmalloc_aligned(4096 * 10);
  *ptr4 = 200;

  kfree(ptr);
  kfree(ptr1);
  kfree(ptr2);
  kfree(ptr3);
  kfree(ptr4);

  // Test kheap expand.
  ptr = (uint8*)kmalloc(32);
  ptr1 = (uint8*)kmalloc(2621374);
  ptr2 = (uint8*)kmalloc(2);

  kheap_validate_print(/* print = */ 1);
}

void kheap_killer() {
  rand_seed(7);
  uint32 size = 500;

  // alloc
  uint8* ptrs[size * 2];
  for (int i = 0; i < size; i++) {
    uint32 random = rand_range(1, 1000);
    //monitor_printf("%u ", random);
    if (i % 5 == 1) {
      ptrs[i] = (uint8*)kmalloc_aligned(random);
    } else {
      ptrs[i] = (uint8*)kmalloc(random);
    }
    ASSERT(kheap_validate_print(0) == (i + 1));
  }

  // free half
  for (int i = 0; i < size / 2; i++) {
    kfree(ptrs[i * 2]);
    ASSERT(kheap_validate_print(0) == (size - (i + 1)));
  }

  // alloc again
  for (int i = 0; i < size; i++) {
    uint32 random = rand_range(1, 1000);
    if (i % 5 >= 2) {
      ptrs[i + size] = (uint8*)kmalloc_aligned(random);
    } else {
      ptrs[i + size] = (uint8*)kmalloc(random);
    }
    ASSERT(kheap_validate_print(0) == size / 2 + (i + 1));
  }

  // free all
  for (int i = 0; i < size / 2; i++) {
    kfree(ptrs[size + i * 2 + 1]);
    ASSERT(kheap_validate_print(0) == (size / 2 * 3 - (i + 1)));
  }
  for (int i = 0; i < size / 2; i++) {
    kfree(ptrs[i * 2 + 1]);
    ASSERT(kheap_validate_print(0) == (size - (i + 1)));
  }
  for (int i = 0; i < size / 2; i++) {
    kfree(ptrs[size + i * 2]);
    ASSERT(kheap_validate_print(0) == (size / 2 - (i + 1)));
  }

  ASSERT(kheap_validate_print(1) == 0);
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

  monitor_println("idle...");
  while(1) {}
}
