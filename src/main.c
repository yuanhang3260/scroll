#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"
#include "interrupt/interrupt.h"

char* welcome = "# welcome to scroll kernel #";

void print_shell() {
  monitor_print_with_color("bash", COLOR_GREEN);
  monitor_print(" > ");
}

int main() {
  monitor_clear();
  monitor_println(welcome);
  //print_shell();

  init_idt();
  //enable_interrupt();
  //init_timer(TIMER_FREQUENCY);

  uint32 *ptr = (uint32*)0xC0900000;
  uint32 do_page_fault = *ptr;

  while(1) {}
}
