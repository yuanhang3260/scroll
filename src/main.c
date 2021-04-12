#include "common/common.h"
#include "monitor/monitor.h"
#include "interrupt/timer.h"

int a = 3;
int b;
char* welcome = "# welcome to scroll kernel #\n";

void print_shell() {
  monitor_write_with_color("bash", COLOR_GREEN);
  monitor_write(" > ");
}

int main() {
  b = 5;
  monitor_clear();
  monitor_write(welcome);
  //print_shell();

  //asm volatile ("int $0x3");
  init_timer(50);

  while(1) {}
}
