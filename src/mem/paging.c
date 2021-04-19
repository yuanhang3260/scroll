#include "mem/paging.h"
#include "monitor/monitor.h"

// 0xC0900000
void* kernel_placement_addr = (void*)KERNEL_START_PLACEMENT_ADDR;

// The kernel's page directory
page_directory_t kernel_page_directory;

// The current page directory;
page_directory_t *current_page_directory = 0;

static bitmap_t phy_frames_map;

void* alloc_kernel_placement_addr(uint32 size) {
  void* result = kernel_placement_addr;
  kernel_placement_addr += size;
  return result;
}

void init_paging() {
  // initialize phy_frames_map
  phy_frames_map.total_bits = PHYSICAL_MEM_SIZE / 4096;
  phy_frames_map.array_size = phy_frames_map.total_bits / 32;
  phy_frames_map.array =
    (uint32*)alloc_kernel_placement_addr(phy_frames_map.array_size * 4);

  // initialize page directory
  kernel_page_directory.page_directory_entries =
    (pde_t*)KERNEL_PAGE_DIR_VIRTUAL_ADDR;
  current_page_directory = &kernel_page_directory;

  // register page fault handler
  register_irq_handler(14, page_fault_handler);
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
  asm volatile("mov %0, %%cr3":: "r"(&dir->page_directory_entries));
  uint32 cr0;
  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000; // Enable paging!
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
    "page fault! (present %d, write %d, user-mode %d, reserved %d, at 0x%x\n",
    present, rw, user_mode, reserved, faulting_address);
}
