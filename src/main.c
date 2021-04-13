#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"

char* welcome = "# welcome to scroll kernel #\n";

void print_shell() {
  monitor_write_with_color("bash", COLOR_GREEN);
  monitor_write(" > ");
}

int main() {
  monitor_clear();
  monitor_write(welcome);
  //print_shell();

  init_idt();
  enable_interrupt();
  init_timer(TIMER_FREQUENCY);

  while(1) {}
}
