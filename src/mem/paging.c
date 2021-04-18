#include "mem/paging.h"

// 0xC0900000
void* kernel_placement_addr = (void*)KERNEL_START_PLACEMENT_ADDR;

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
