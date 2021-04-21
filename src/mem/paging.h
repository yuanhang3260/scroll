#ifndef MEM_PAGING_H
#define MEM_PAGING_H

#include "common/common.h"
#include "common/global.h"
#include "utils/bitmap.h"
#include "interrupt/interrupt.h"

#define KERNEL_VIRTUAL_ADDR_START    0xC0800000
#define KERNEL_PHYSICAL_ADDR_START   0x300000
#define KERNEL_SIZE_MAX              (1024 * 1024)
#define KERNEL_START_PLACEMENT_ADDR  \
  (KERNEL_VIRTUAL_ADDR_START + KERNEL_SIZE_MAX)


#define KERNEL_PAGE_DIR_VIRTUAL_ADDR  0xC0400000
#define KERNEL_PAGE_DIR_VIRTUAL_ADDR  0xC0400000

// ************************************************************************** //
// 4 byte
typedef struct page_table_entry {
  uint32 present    : 1;   // Page present in memory
  uint32 rw         : 1;   // Read-only if clear, readwrite if set
  uint32 user       : 1;   // Supervisor level only if clear
  uint32 accessed   : 1;   // Has the page been accessed since last refresh?
  uint32 dirty      : 1;   // Has the page been written to since last refresh?
  uint32 unused     : 7;   // Amalgamation of unused and reserved bits
  uint32 frame      : 20;  // Frame address (shifted right 12 bits)
} pte_t;

typedef pte_t pde_t;

// 4KB
typedef struct page_table {
  pte_t* page_table_entries;  // [1024]
} page_table_t;

// 4KB
typedef struct page_directory {
  pde_t* page_directory_entries;  // [1024]
} page_directory_t;


// ************************************************************************** //
void init_paging();

void* alloc_kernel_placement_addr(uint32 size);

int32 allocate_phy_frame();
void release_phy_frame(uint32 frame);

void switch_page_directory(page_directory_t *new);

void page_fault_handler(isr_params_t params);

#endif
