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


// ********************* virtual memory layout *********************************
// 0xC0000000 ... 0xC0100000 ... 0xC0400000  boot & reserverd                4MB
// 0xC0400000 ... 0xC0800000 page tables, 0xC0401000 page directory          4MB
// 0xC0800000 ... 0xC0900000 kernel load                                     1MB
#define PAGE_DIR_VIRTUAL        0xC0401000
#define PAGE_TABLES_VIRTUAL     0xC0400000

// ********************* physical memory layout ********************************
// 0x00000000 ... 0x00100000  boot & reserved                                1MB
// 0x00100000 ... 0x00200000  kernel page tables                             1MB
// 0x00200000 ... 0x00300000  kernel load                                    1MB
#define KERNEL_PAGE_DIR_PHY      0x00100000

// *****************************************************************************
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
typedef struct page_directory {
  pde_t* page_dir_entries_phy;  // [1024]
} page_directory_t;


// *****************************************************************************
void init_paging();

void* alloc_kernel_placement_addr(uint32 size);

int32 allocate_phy_frame();
void release_phy_frame(uint32 frame);

void switch_page_directory(page_directory_t *new);

void page_fault_handler(isr_params_t params);

#endif
