#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"

char* welcome = "# welcome to scroll kernel #";

void print_shell() {
  monitor_print_with_color("bash", COLOR_GREEN);
  monitor_print(" > ");
}

uint64 foo(char a, uint16 b, uint32 c, uint64 d) {
  uint64 e = a + b + c + d;
  return e;
}

int main() {
  monitor_clear();
  monitor_println(welcome);
  //print_shell();

  init_idt();
  enable_interrupt();
  init_timer(TIMER_FREQUENCY);

  uint64 e = foo('x', 1, 2, 3L);

  while(1) {}
}
