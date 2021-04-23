#include "mem/paging.h"
#include "monitor/monitor.h"

// 0xC0900000
void* kernel_placement_addr = (void*)KERNEL_START_PLACEMENT_ADDR;

// kernel's page directory
page_directory_t kernel_page_directory;

// the current page directory;
page_directory_t *current_page_directory = 0;

static bitmap_t phy_frames_map;
uint32 bitarray[PHYSICAL_MEM_SIZE / 4096 / 32];

void* alloc_kernel_placement_addr(uint32 size) {
  void* result = kernel_placement_addr;
  kernel_placement_addr += size;
  return result;
}

void init_paging() {
  // initialize phy_frames_map
  // we have already used the first 3MB phyical memory.
  phy_frames_map.total_bits = PHYSICAL_MEM_SIZE / 4096;
  phy_frames_map.array_size = phy_frames_map.total_bits / 32;
  for (int i = 0; i < 3 * 1024 * 1024 / 4096 / 32; i++) {
    bitarray[i] = 0xFFFFFFFF;
  }
  phy_frames_map.array = bitarray;

  // initialize page directory
  kernel_page_directory.page_dir_entries_phy = (pde_t*)KERNEL_PAGE_DIR_PHY;
  current_page_directory = &kernel_page_directory;

  // register page fault handler
  register_interrupt_handler(14, page_fault_handler);
}

int32 allocate_phy_frame() {
  uint32 frame;
  if (!allocate_first_free(&phy_frames_map, &frame)) {
    return -1;
  }

  return (int32)frame;
}

void release_phy_frame(uint32 frame) {
  clear_bit(&phy_frames_map, frame);
}

void switch_page_directory(page_directory_t *dir) {
  current_page_directory = dir;
  asm volatile("mov %0, %%cr3":: "r"(dir->page_dir_entries_phy));

  // enable paging!
  uint32 cr0;
  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void page_fault_handler(isr_params_t params) {
  // The faulting address is stored in the CR2 register
  uint32 faulting_address;
  asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

  // The error code gives us details of what happened.
  // page not present?
  int present = params.err_code & 0x1;
  // write operation?
  int rw = params.err_code & 0x2;
  // processor was in user-mode?
  int user_mode = params.err_code & 0x4;
  // overwritten CPU-reserved bits of page entry?
  int reserved = params.err_code & 0x8;
  // caused by an instruction fetch?
  int id = params.err_code & 0x10;

  // handle page fault
  monitor_printf(
    "page fault: %x, present %d, write %d, user-mode %d, reserved %d\n",
    faulting_address, present, rw, user_mode, reserved);

  PANIC("");
}

static void allcoate_page(uint32 virtual_addr) {
  // Lookup pde - note we use virtual address 0xC0401000 to access page
  // directory, which is the actually the 2nd page table.
  uint32 pde_index = virtual_addr >> 22;
  pde_t* pd = (pde_t*)PAGE_DIR_VIRTUAL;
  pde_t* pde = pd + pde_index;

  // Allcoate page table for this pde, if needed.
  if (!pde->present) {
    int32 frame = allocate_phy_frame();
    if (frame < 0) {
      PANIC("couldn't find any free physical frame for new page table");
    }
    pde->present = 1;
    pde->rw = 1;
    pde->user = 1;
    pde->frame = frame;
  }

  // Lookup pte - still with virtual address. Note all 1024 page tables are
  // continuous in virtual space, from 0xC0400000 to 0xC0800000.
  uint32 pte_index = virtual_addr >> 12;
  pte_t* kernel_page_tables_virtual = (pte_t*)PAGE_TABLES_VIRTUAL;
  pte_t* pte = kernel_page_tables_virtual + pte_index;
  if (!pte->present) {
    int32 frame = allocate_phy_frame();
    if (frame < 0) {
      PANIC("couldn't find any free physical frame for requested address");
    }
    pte->present = 1;
    pte->rw = 1;
    pte->user = 1;
    pte->frame = frame;
  }
  
}
