#ifndef MEM_KHEAP_H
#define MEM_KHEAP_H

#include "common/common.h"
#include "utils/ordered_array.h"

#define KHEAP_INITIAL_SIZE  0x100000

#define HEAP_INDEX_NUM      0x20000
#define HEAP_MAGIC          0x123890AB
#define HEAP_MIN_SIZE       0x10000  // 64KB

typedef struct kheap_block_header {
  uint32 magic;
  uint8 is_hole;
  uint32 size;
} kheap_block_header_t;

typedef struct {
  uint32 magic;
  kheap_block_header_t *kheap_block_header;
} kheap_block_footer_t;

typedef struct kernel_heap {
  ordered_array_t index;
  uint32 start_address;
  uint32 end_address;
  uint32 size;
  uint32 max_address;
  uint8 supervisor;
  uint8 readonly;
} kheap_t;

void init_kheap();

kheap_t create_kheap(uint32 start, uint32 end, uint32 max, uint8 supervisor, uint8 readonly);

void* kmalloc(uint32 size);

void* kmalloc_aligned(uint32 size);

void kfree(void *p);

#endif
