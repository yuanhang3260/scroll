#include "common/stdlib.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "monitor/monitor.h"
#include "utils/debug.h"

// kernel's page directory
page_directory_t kernel_page_directory;

// the current page directory;
page_directory_t* current_page_directory = 0;

static bitmap_t phy_frames_map;
uint32 bitarray[PHYSICAL_MEM_SIZE / PAGE_SIZE / 32];

void init_paging() {
  // Initialize phy_frames_map. Note we have already used the first 3MB and last 4KB
  // phyical memory for kernel initialization.
  //
  // totally 8192 frames
  for (int i = 0; i < 3 * 1024 * 1024 / PAGE_SIZE / 32; i++) {
    bitarray[i] = 0xFFFFFFFF;
  }
  phy_frames_map = bitmap_create(bitarray, PHYSICAL_MEM_SIZE / PAGE_SIZE);
  bitmap_clear_bit(&phy_frames_map, PHYSICAL_MEM_SIZE / PAGE_SIZE - 1);

  // Initialize page directory.
  kernel_page_directory.page_dir_entries_phy = KERNEL_PAGE_DIR_PHY;
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

void switch_page_directory(page_directory_t* dir) {
  current_page_directory = dir;
  asm volatile("mov %0, %%cr3":: "r"(dir->page_dir_entries_phy));

  // enable paging!
  uint32 cr0;
  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void reload_page_directory(page_directory_t *dir) {
  current_page_directory = dir;
  asm volatile("mov %0, %%cr3":: "r"(dir->page_dir_entries_phy));
}

page_directory_t* get_crt_page_directory() {
  return current_page_directory;
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

  map_page(faulting_address);
  reload_page_directory(current_page_directory);
}

void map_page(uint32 virtual_addr) {
  map_page_with_frame(virtual_addr, -1);
}

void map_page_with_frame(uint32 virtual_addr, int32 frame) {
  // Lookup pde - note we use virtual address 0xC0701000 to access page
  // directory, which is the actually the 2nd page table of kernel space.
  uint32 pde_index = virtual_addr >> 22;
  pde_t* pd = (pde_t*)PAGE_DIR_VIRTUAL;
  pde_t* pde = pd + pde_index;

  // Allcoate page table for this pde, if needed.
  if (!pde->present) {
    int32 page_table_frame = allocate_phy_frame();
    if (page_table_frame < 0) {
      monitor_printf("couldn't alloc frame for page table on %d\n", pde_index);
      PANIC();
    }
    //monitor_printf("alloca frame %d for page table %d\n", frame, pde_index);
    pde->present = 1;
    pde->rw = 1;
    pde->user = 1;
    pde->frame = page_table_frame;
  }

  // Lookup pte - still use virtual address. Note all 1024 page tables are
  // continuous in virtual space, from 0xC0400000 to 0xC0800000.
  uint32 pte_index = virtual_addr >> 12;
  pte_t* kernel_page_tables_virtual = (pte_t*)PAGE_TABLES_VIRTUAL;
  pte_t* pte = kernel_page_tables_virtual + pte_index;
  if (!pte->present) {
    if (frame < 0) {
      frame = allocate_phy_frame();
      if (frame < 0) {
        monitor_printf("couldn't alloc frame for addr %x\n", virtual_addr);
        PANIC();
      }
    }
    //monitor_printf("alloc frame %d for virtual addr %x\n", frame, virtual_addr);
    pte->present = 1;
    pte->rw = 1;
    pte->user = 1;
    pte->frame = frame;
  }
}

void release_page(uint32 virtual_addr) {
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

void release_pages(uint32 virtual_addr, uint32 pages) {
  virtual_addr = (virtual_addr / PAGE_SIZE) * PAGE_SIZE;
  for (uint32 i = 0; i < pages; i++) {
    release_page(virtual_addr + i * PAGE_SIZE);
  }

  reload_page_directory(current_page_directory);
}

// Copy the entire page dir and its page tables:
//  - 256 kernel page tables are shared;
//  - user space page tables are copied, and page entries are marked copy-on-write;
//
// Return the physical address of new page dir.
page_directory_t clone_crt_page_dir() {
  int32 new_pd_frame = allocate_phy_frame();
  if (new_pd_frame < 0) {
    monitor_printf("couldn't alloc frame for new page dir\n");
    PANIC();
  }

  // Map copied page tables (including the new page dir) to some virtual space so that
  // we can access them - let's use the last 2 virtual pages (0xFFFFE000 - 0xFFFFFFFF), for the
  // new page dir and the page table that is being copied, respectively.
  //
  // First, map the new page dir.
  map_page_with_frame(COPIED_PAGE_DIR_VADDR, new_pd_frame);
  reload_page_directory(current_page_directory);
  memset((void*)COPIED_PAGE_DIR_VADDR, 0, PAGE_SIZE);

  pde_t* new_pd = (pde_t*)COPIED_PAGE_DIR_VADDR;
  pde_t* crt_pd = (pde_t*)PAGE_DIR_VIRTUAL;
  // Share all 256 kernel pdes - except pde 769, which should points to new page dir itself.
  for (uint32 i = 768; i < 1024; i++) {
    pde_t* new_pde = new_pd + i;
    if (i == 769) {
      new_pde->present = 1;
      new_pde->rw = 1;
      new_pde->user = 1;
      new_pde->frame = new_pd_frame;
    } else {
      *new_pde = *(crt_pd + i);
    }
  }

  // Copy user space page tables.
  for (uint32 i = 0; i < 768; i++) {
    pde_t* crt_pde = crt_pd + i;
    if (!crt_pde->present) {
      continue;
    }

    // Alloc a new frame for copied page table.
    int32 new_pt_frame = allocate_phy_frame();
    if (new_pt_frame < 0) {
      monitor_printf("couldn't alloc frame for copied page table\n");
      PANIC();
    }

    // Copy page table and set its ptes copy-on-write.
    map_page_with_frame(COPIED_PAGE_TABLE_VADDR, new_pt_frame);
    reload_page_directory(current_page_directory);
    memcpy((void*)COPIED_PAGE_TABLE_VADDR, (void*)(PAGE_TABLES_VIRTUAL + i * PAGE_SIZE), PAGE_SIZE);
    for (int j = 0; j < 1024; j++) {
      pte_t* pte = (pte_t*)COPIED_PAGE_TABLE_VADDR + i;
      if (!pte->present) {
        continue;
      }
      // copy-on-write
      pte->rw = 0;
    }

    // Set page dir entry.
    pde_t* new_pde = new_pd + i;
    new_pde->present = 1;
    new_pde->rw = 1;
    new_pde->user = 1;
    new_pde->frame = new_pt_frame;
  }

  // Release mapping for new page tables on current process.
  release_pages(COPIED_PAGE_DIR_VADDR, 2);

  page_directory_t page_directory;
  page_directory.page_dir_entries_phy = new_pd_frame * PAGE_SIZE;
  return page_directory;
}

// ******************************** unit tests **********************************
void memory_killer() {
  uint32 *ptr = (uint32*)0xC0900000;
  while (1) {
    *ptr = 3;
    ptr += 1;
  }
}
