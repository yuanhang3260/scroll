#ifndef COMMON_GLOBAL_H
#define COMMON_GLOBAL_H

#include "common/common.h"

// ***************************** gdt & idt ********************************** //
#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_K_CODE   ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA   ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK  SELECTOR_K_DATA
#define SELECTOR_K_GS     ((3 << 3) + (TI_GDT << 2) + RPL0)  // video segment

#define IDT_GATE_P     1
#define IDT_GATE_DPL0  0
#define IDT_GATE_DPL3  3
#define IDT_GATE_32_TYPE  0xE

#define IDT_GATE_ATTR_DPL0 \
  ((IDT_GATE_P << 7) + (IDT_GATE_DPL0 << 5) + IDT_GATE_32_TYPE)

#define IDT_GATE_ATTR_DPL3 \
  ((IDT_GATE_P << 7) + (IDT_GATE_DPL3 << 5) + IDT_GATE_32_TYPE)


// ****************************** memory ************************************ //
#define PHYSICAL_MEM_SIZE 33554432  // 32MB

#endif
