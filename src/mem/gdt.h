#ifndef MEM_GDT_H
#define MEM_GDT_H

#include "common/common.h"

#define FLAG_G_4K  (1 << 3)
#define FLAG_D_32  (1 << 2)

#define DESC_P     (1 << 7)

#define DESC_DPL_0   (0 << 5)
#define DESC_DPL_1   (1 << 5)
#define DESC_DPL_2   (2 << 5)
#define DESC_DPL_3   (3 << 5)

#define DESC_S_CODE   (1 << 4)
#define DESC_S_DATA   (1 << 4)
#define DESC_S_SYS    (0 << 4)

#define DESC_TYPE_CODE  0xA   // r/x non-conforming code segment
#define DESC_TYPE_DATA  0x2   // r/w data segment

struct gdt_ptr {
  uint16 limit;
  uint32 base;
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

struct gdt_entry {
  uint16 limit_low;
  uint16 base_low;
  uint8  base_middle;
  uint8  access;
  uint8  attributes;
  uint8  base_high;
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;


// ****************************************************************************
void init_gdt();

void gdt_set_gate(int32 num, uint32 base, uint32 limit, uint8 access, uint8 flags);

#endif
