#include "common/global.h"
#include "common/io.h"
#include "common/stdlib.h"
#include "monitor/monitor.h"
#include "interrupt/interrupt.h"

extern void reload_idt(uint32);
static void set_idt_gate(uint8 num, uint32 base, uint16 sel, uint8 attrs);

idt_ptr_t idt_ptr;
static idt_entry_t idt_entries[256];

void init_idt() {
  idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
  idt_ptr.base = (uint32)(&idt_entries);

  memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);

  // TODO: use IDT_GATE_ATTR_DPL3 in user mode
  set_idt_gate(0, (uint32)isr0 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(1, (uint32)isr1 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(2, (uint32)isr2 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(3, (uint32)isr3 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(4, (uint32)isr4 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(5, (uint32)isr5 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(6, (uint32)isr6 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(7, (uint32)isr7 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(8, (uint32)isr8 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(9, (uint32)isr9 , SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(10, (uint32)isr10, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(11, (uint32)isr11, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(12, (uint32)isr12, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(13, (uint32)isr13, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(14, (uint32)isr14, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(15, (uint32)isr15, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(16, (uint32)isr16, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(17, (uint32)isr17, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(18, (uint32)isr18, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(19, (uint32)isr19, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(20, (uint32)isr20, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(21, (uint32)isr21, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(22, (uint32)isr22, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(23, (uint32)isr23, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(24, (uint32)isr24, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(25, (uint32)isr25, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(26, (uint32)isr26, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(27, (uint32)isr27, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(28, (uint32)isr28, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(29, (uint32)isr29, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(30, (uint32)isr30, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);
  set_idt_gate(31, (uint32)isr31, SELECTOR_K_CODE, IDT_GATE_ATTR_DPL0);

  // refresh idt
  reload_idt((uint32)&idt_ptr);
}

static void set_idt_gate(uint8 num, uint32 base, uint16 sel, uint8 attrs) {
  idt_entries[num].handler_addr_low = base & 0xFFFF;
  idt_entries[num].handler_addr_high = (base >> 16) & 0xFFFF;
  idt_entries[num].sel = sel;
  idt_entries[num].always0 = 0;
  idt_entries[num].attrs = attrs;
}

// This gets called from our ASM interrupt handler stub.
void isr_handler(isr_params_t regs) {
  monitor_write("recieved interrupt: ");
  monitor_write_dec(regs.int_no);
  monitor_put('\n');
}
