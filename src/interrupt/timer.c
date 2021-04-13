#include "common/io.h"
#include "monitor/monitor.h"
#include "interrupt/interrupt.h"
#include "interrupt/timer.h"

uint32 tick = 0;

static void timer_callback(isr_params_t regs) {
  if (tick % TIMER_FREQUENCY == 0) {
    monitor_write("second: ");
    monitor_write_dec(tick / TIMER_FREQUENCY);
    monitor_write("\n");
  }
  tick++;
}

void init_timer(uint32 frequency){
  // Firstly, register our timer callback.
  register_irq_handler(IRQ0_INT_NUM, &timer_callback);

  // The value we send to the PIT is the value to divide it's input clock
  // (1193180 Hz) by, to get our required frequency. Important to note is
  // that the divisor must be small enough to fit into 16-bits.
  uint32 divisor = 1193180 / frequency;

  // Send the command byte.
  outb(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  uint8 l = (uint8)(divisor & 0xFF);
  uint8 h = (uint8)((divisor>>8) & 0xFF);

  // Send the frequency divisor.
  outb(0x40, l);
  outb(0x40, h);
}
