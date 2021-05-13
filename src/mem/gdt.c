#include "mem/gdt.h"

static gdt_ptr_t gdt_ptr;
static gdt_entry_t gdt_entries[6];

void init_gdt() {
  gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
  gdt_ptr.base = (uint32)&gdt_entries;

  // reserved
  gdt_set_gate(0, 0, 0, 0, 0);

  // kernel code
  gdt_set_gate(1, 0, 0xFFFFF, DESC_P | DESC_DPL_0 | DESC_TYPE_CODE | DESC_TYPE_CODE, FLAG_G_4K | FLAG_D_32);
  // kernel data
  gdt_set_gate(2, 0, 0xFFFFF, DESC_P | DESC_DPL_0 | DESC_TYPE_DATA | DESC_TYPE_DATA, FLAG_G_4K | FLAG_D_32);
  // video: only 8 pages
  gdt_set_gate(3, 0, 7, DESC_P | DESC_DPL_0 | DESC_TYPE_DATA | DESC_TYPE_DATA, FLAG_G_4K | FLAG_D_32);
  // user code
  gdt_set_gate(4, 0, 0xFFFFF, DESC_P | DESC_DPL_3 | DESC_TYPE_DATA | DESC_TYPE_DATA, FLAG_G_4K | FLAG_D_32);
  // user data
  gdt_set_gate(5, 0, 0xFFFFF, DESC_P | DESC_DPL_3 | DESC_TYPE_CODE | DESC_TYPE_CODE, FLAG_G_4K | FLAG_D_32);
}

void gdt_set_gate(int32 num, uint32 base, uint32 limit, uint8 access, uint8 flags) {
  gdt_entries[num].limit_low = (limit & 0xFFFF);
  gdt_entries[num].base_low = (base & 0xFFFF);
  gdt_entries[num].base_middle = (base >> 16) & 0xFF;
  gdt_entries[num].access = access;
  gdt_entries[num].attributes = (limit >> 16) & 0x0F;
  gdt_entries[num].attributes |= ((flags << 4) & 0xF0);
  gdt_entries[num].base_high = (base >> 24) & 0xFF;
}
