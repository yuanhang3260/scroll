#ifndef MEM_PAGING_H
#define MEM_PAGING_H

#include "common/common.h"
#include "common/global.h"
#include "utils/bitmap.h"

#define KERNEL_VIRTUAL_ADDR_START    0xC0800000
#define KERNEL_PHYSICAL_ADDR_START   0x300000
#define KERNEL_SIZE_MAX              (1024 * 1024)
#define KERNEL_START_PLACEMENT_ADDR  \
  (KERNEL_VIRTUAL_ADDR_START + KERNEL_SIZE_MAX)

void init_paging();

void* alloc_kernel_placement_addr(uint32 size);

int32 allocate_phy_frame();
void release_phy_frame(uint32 frame);

#endif
