#include "common/global.h"
#include "common/io.h"
#include "common/stdlib.h"
#include "monitor/monitor.h"
#include "interrupt/irq.h"

isr_t interrupt_handlers[256];

// This gets called from our ASM interrupt handler stub.
void irq_handler(interrupt_params_t params) {
  // Send an EOI (end of interrupt) signal to the PICs.
  // If this interrupt involved the slave.
  if (params.int_num >= 40) {
    // Send reset signal to slave.
    outb(0xA0, 0x20);
  }
  
  // Send reset signal to master.
  outb(0x20, 0x20);

  if (interrupt_handlers[params.int_num] != 0) {
    isr_t handler = interrupt_handlers[params.int_num];
    handler(params);
  }
}

void register_interrupt_handler(uint8 n, isr_t handler) {
  interrupt_handlers[n] = handler;
}
