#include "mem/paging.h"
#include "monitor/monitor.h"
#include "utils/debug.h"

// 0xC0900000
void* kernel_placement_addr = (void*)KERNEL_PLACEMENT_ADDR_START;

// kernel's page directory
page_directory_t kernel_page_directory;

// the current page directory;
page_directory_t *current_page_directory = 0;

static bitmap_t phy_frames_map;
uint32 bitarray[PHYSICAL_MEM_SIZE / PAGE_SIZE / 32];

static void allcoate_page(uint32 virtual_addr);
static void release_page(uint32 virtual_addr);
static void release_pages(uint32 virtual_addr, uint32 pages);

void init_paging() {
  // Initialize phy_frames_map. Note we have already used the first 3MB
  // phyical memory for kernel initialization.
  //
  // totally 8192 frames
  for (int i = 0; i < 3 * 1024 * 1024 / PAGE_SIZE / 32; i++) {
    bitarray[i] = 0xFFFFFFFF;
  }
  phy_frames_map = bitmap_create(bitarray, PHYSICAL_MEM_SIZE / PAGE_SIZE);

  // Initialize page directory.
  kernel_page_directory.page_dir_entries_phy = (pde_t*)KERNEL_PAGE_DIR_PHY;
  current_page_directory = &kernel_page_directory;

  // Release memory for loading kernel binany - it's no longer needed.
  release_pages(0xFFFFFFFF - KERNEL_BIN_LOAD_SIZE + 1, KERNEL_BIN_LOAD_SIZE / PAGE_SIZE);

  // Register page fault handler.
  register_interrupt_handler(14, page_fault_handler);
}

int32 allocate_phy_frame() {
  uint32 frame;
  if (!bitmap_allocate_first_free(&phy_frames_map, &frame)) {
    return -1;
  }

  return (int32)frame;
}

void release_phy_frame(uint32 frame) {
  bitmap_clear_bit(&phy_frames_map, frame);
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

void reload_page_table(page_directory_t *dir) {
  asm volatile("mov %0, %%cr3":: "r"(dir->page_dir_entries_phy));
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
  //monitor_printf(
  //  "page fault: %x, present %d, write %d, user-mode %d, reserved %d\n",
  //  faulting_address, present, rw, user_mode, reserved);

  allcoate_page(faulting_address);
  reload_page_table(current_page_directory);
}

static void allcoate_page(uint32 virtual_addr) {
  // Lookup pde - note we use virtual address 0xC0701000 to access page
  // directory, which is the actually the 2nd page table of kernel space.
  uint32 pde_index = virtual_addr >> 22;
  pde_t* pd = (pde_t*)PAGE_DIR_VIRTUAL;
  pde_t* pde = pd + pde_index;

  // Allcoate page table for this pde, if needed.
  if (!pde->present) {
    int32 frame = allocate_phy_frame();
    if (frame < 0) {
      monitor_printf("couldn't alloc frame for page table on %d\n", pde_index);
      PANIC();
    }
    monitor_printf("alloca frame %d for page table %d\n", frame, pde_index);
    pde->present = 1;
    pde->rw = 1;
    pde->user = 1;
    pde->frame = frame;
  }

  // Lookup pte - still use virtual address. Note all 1024 page tables are
  // continuous in virtual space, from 0xC0400000 to 0xC0800000.
  uint32 pte_index = virtual_addr >> 12;
  pte_t* kernel_page_tables_virtual = (pte_t*)PAGE_TABLES_VIRTUAL;
  pte_t* pte = kernel_page_tables_virtual + pte_index;
  if (!pte->present) {
    int32 frame = allocate_phy_frame();
    if (frame < 0) {
      monitor_printf("couldn't alloc frame for addr %x\n", virtual_addr);
      PANIC();
    }
    //monitor_printf("alloc frame %d for virtual addr %x\n", frame, virtual_addr);
    pte->present = 1;
    pte->rw = 1;
    pte->user = 1;
    pte->frame = frame;
  }
}

static void release_page(uint32 virtual_addr) {
  uint32 pde_index = virtual_addr >> 22;
  pde_t* pd = (pde_t*)PAGE_DIR_VIRTUAL;
  pde_t* pde = pd + pde_index;

  if (!pde->present) {
    monitor_printf("%x page table not present\n", virtual_addr);
    PANIC();
  }

  // reset pte
  uint32 pte_index = virtual_addr >> 12;
  pte_t* kernel_page_tables_virtual = (pte_t*)PAGE_TABLES_VIRTUAL;
  pte_t* pte = kernel_page_tables_virtual + pte_index;
  *((uint32*)pte) = 0;
}

static void release_pages(uint32 virtual_addr, uint32 pages) {
  virtual_addr = (virtual_addr / PAGE_SIZE) * PAGE_SIZE;
  for (uint32 i = 0; i < pages; i++) {
    release_page(virtual_addr + i * PAGE_SIZE);
  }

  reload_page_table(current_page_directory);
}


// ******************************** unit tests **********************************
void memory_killer() {
  uint32 *ptr = (uint32*)KERNEL_PLACEMENT_ADDR_START;
  while(1) {
    *ptr = 3;
    ptr += 1;
  }
}
