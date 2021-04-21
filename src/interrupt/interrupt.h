#ifndef INTERRUPT_INTERRUPT_H
#define INTERRUPT_INTERRUPT_H

#include "common/common.h"
#include "common/global.h"

// ******************************** idt ************************************* //
struct idt_entry_struct {
  // the lower 16 bits of the handler address
  uint16 handler_addr_low;
  // kernel segment selector
  uint16 sel;
  // this must always be zero
  uint8 always0;
  // attribute flags
  uint8 attrs;
  // The upper 16 bits of the handler address
  uint16 handler_addr_high;
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr_struct {
  uint16 limit;
  uint32 base;
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

void init_idt();

// ******************************** isr ************************************* //
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// argument struct for common isr_handler
typedef struct isr_params {
  uint32 ds;
  uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32 int_num;
  uint32 err_code;
  uint32 eip, cs, eflags, useresp, ss;
} isr_params_t;

typedef void (*isr_t)(isr_params_t);
void isr_handler(isr_params_t regs);
void register_interrupt_handler(uint8 n, isr_t handler);

// ******************************** irq ************************************* //
#define IRQ0_INT_NUM 32
#define IRQ1_INT_NUM 33
#define IRQ2_INT_NUM 34
#define IRQ3_INT_NUM 35
#define IRQ4_INT_NUM 36
#define IRQ5_INT_NUM 37
#define IRQ6_INT_NUM 38
#define IRQ7_INT_NUM 39
#define IRQ8_INT_NUM 40
#define IRQ9_INT_NUM 41
#define IRQ10_INT_NUM 42
#define IRQ11_INT_NUM 43
#define IRQ12_INT_NUM 44
#define IRQ13_INT_NUM 45
#define IRQ14_INT_NUM 46
#define IRQ15_INT_NUM 47

extern void irq0 ();
extern void irq1 ();
extern void irq2 ();
extern void irq3 ();
extern void irq4 ();
extern void irq5 ();
extern void irq6 ();
extern void irq7 ();
extern void irq8 ();
extern void irq9 ();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

void enable_interrupt();
void disable_interrupt();

#endif