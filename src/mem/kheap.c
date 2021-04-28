#include "mem/kheap.h"
#include "mem/paging.h"
#include "utils/debug.h"

static kheap_t *kheap = 0;

#define HEADER_SIZE (sizeof(kheap_block_header_t))
#define FOOTER_SIZE (sizeof(kheap_block_footer_t))
#define BLOCK_META_SIZE (sizeof(kheap_block_header_t) + sizeof(kheap_block_footer_t))

#define IS_HOLE   1
#define NOT_HOLE  0

// ************************************************************************
static void* kmalloc_impl(uint32 size, int align) {
  return alloc(kheap, size, (uint8)align);
}

void* kmalloc(uint32 size) {
  return kmalloc_impl(size, 0);
}

void* kmalloc_aligned(uint32 size) {
  return kmalloc_impl(size, 1);
}

void kfree(void *p) {
  free(kheap, p);
}

static void kheap_expand(kheap_t *this, uint32 new_size) {
  // Make it page aligned.
  if (new_size & 0xFFFFF000 != 0) {
    new_size = (new_size & 0xFFFFF000) + PAGE_SIZE;
  }

  ASSERT(new_size > this->size);

  uint32 new_end = this->start_address + new_size;
  ASSERT(new_end <= this->max_address);
  this->size = new_size;
  this->end_address = new_end;
}

static uint32 kheap_contract(kheap_t *this, uint32 new_size) {
  // Get the nearest following page boundary.
  if (new_size & 0xFFFFF000 != 0) {
    new_size = (new_size & 0xFFFFF000) + PAGE_SIZE;
  }

  // Sanity check.
  ASSERT(new_size < this->size);

  if (new_size < HEAP_MIN_SIZE) {
    new_size = HEAP_MIN_SIZE;
  }

  this->size = new_size;
  this->end_address = this->start_address + new_size;
  return new_size;
}

static kheap_block_header_t* make_block(uint32 start, uint32 end, uint8 is_hole) {
  int32 size = end - start - BLOCK_META_SIZE;
  ASSERT(size > 0);

  kheap_block_header_t* block_header = (kheap_block_header_t*)start;
  block_header->magic = HEAP_MAGIC;
  block_header->size = size;
  block_header->is_hole = is_hole;

  kheap_block_footer_t* block_footer = (kheap_block_footer_t*)(end - FOOTER_SIZE);
  block_footer->magic = HEAP_MAGIC;
  block_footer->header = block_header;

  return block_header;
}

static int8 kheap_block_comparator(void* x, void * y) {
  uint32 size1 = ((kheap_block_header_t*)x)->size;
  uint32 size2 = ((kheap_block_header_t*)y)->size;
  return  size1 < size2 ? -1 : (size1 == size2 ? 0 : 1);
}

kheap_t create_kheap(uint32 start, uint32 end, uint32 max, uint8 supervisor, uint8 readonly) {
  ASSERT(start & 0xFFF == 0);
  ASSERT(end & 0xFFF == 0);

  kheap_t kheap;

  // Initialise the index.
  kheap.index = ordered_array_create(
      (type_t*)start, HEAP_INDEX_NUM, &kheap_block_comparator);

  // Shift the start address forward to resemble where we can start putting data.
  start += ((sizeof(type_t) * HEAP_INDEX_NUM));

  // Write the start, end and max addresses into the heap structure.
  kheap.start_address = start;
  kheap.end_address = end;
  kheap.size = end - start;
  kheap.max_address = max;
  kheap.supervisor = supervisor;
  kheap.readonly = readonly;

  // Start off with one large hole in the index.
  make_block(start, end, IS_HOLE);
  ordered_array_insert(&kheap.index, (type_t)start);

  return kheap;
}

// Find the smallest hole that fits.
static int32 find_smallest_hole(kheap_t *this, uint32 size, uint8 page_align) {
  uint32 iterator = 0;
  for (int i = 0; i < this->index.size; i++) {
    kheap_block_header_t* header = (kheap_block_header_t*)ordered_array_get(&this->index, i);
    if (page_align) {
      // Align the starting point.
      // |..................|..................|..................|  page align
      //      |h| data  |f|h| data |f|
      uint32 start = (uint32)header + HEADER_SIZE;
      uint32 next_page_align_addr = start;
      if (start & 0xFFF != 0) {
        next_page_align_addr = (start & 0xFFFFF000) + PAGE_SIZE;
      }
      if (next_page_align_addr < start + header->size &&
            next_page_align_addr - start > BLOCK_META_SIZE) {
        return i;
      }
    } else if (header->size >= size) {
      return i;
    }
  }

  return -1;
}

void *alloc(kheap_t *this, uint32 size, uint8 page_align) {
  uint32 requested_size = size + BLOCK_META_SIZE;
  int32 iterator = find_smallest_hole(requested_size, page_align, heap);
  if (iterator < 0) {
    // No free hole fits, we need to expand the heap.
    uint32 old_capacity = this->end_address - this->start_address;
    uint32 old_end_address = this->end_address;
    kheap_expand(this, old_capacity + requested_size);
    uint32 new_capacity = this->end_address - this->start_address;

    int32 last_block_idx = -1;
    uint32 value = 0;
    for (int32 i = 0; i < this->index.size; i++) {
      uint32 tmp = (uint32)ordered_array_get(&this->index, i);
      if (tmp > value) {
        value = tmp;
        last_block_idx = i;
      }
    }

    uint32 extended_size = new_capacity - old_capacity;
    // If there is no free hole, we need to add one.
    if (last_block_idx == -1) {
      kheap_block_header_t* new_header = make_block(old_end_address, extended_size, IS_HOLE);
      ordered_array_insert(&kheap->index, (type_t)new_header);
    } else {
      // Extend the last hole.
      kheap_block_header_t *header = lookup_ordered_array(&this->index, last_block_idx);
      make_block((uint32)header, header->size + extended_size, IS_HOLE);
    }

    // Now try alloc again.
    return alloc(size, page_align, heap);
  }

  kheap_block_header_t *header =
      (kheap_block_header_t*)lookup_ordered_array(&kheap->index, iterator);
  uint32 header_pos = (uint32)header;
  uint32 block_size = header->size;
  // If the remaining size cannot hold a block, then do not make new hole.
  if (block_size - requested_size <= BLOCK_META_SIZE) {
    size += (block_size - requested_size);
    requested_size = block_size;
  }

  // If we need to page-align the data, move the header to align data,
  // and there may be space in the front that can make a new hole.
  if (page_align && (header_pos & 0xFFFFF000)) {
    // |..................|..................|..................|  page align
    //      |h| data  |f|h| data |f|
    uint32 cutSize = PAGE_SIZE - (header_pos & 0xFFF) - HEADER_SIZE;
    kheap_block_header_t* cut_hole_header = make_block(header_pos, cutSize, IS_HOLE);

    // uint32 new_block_end =
    //     (header_pos & 0xFFFFF000) + PAGE_SIZE - HEADER_SIZE;
    // kheap_block_header_t* new_hole_header = (kheap_block_header_t*)header_pos;
    // new_hole_header->size = PAGE_SIZE - (header_pos & 0xFFF) - HEADER_SIZE;
    // new_hole_header->magic = HEAP_MAGIC;
    // new_hole_header->is_hole  = 1;

    // kheap_block_footer_t* hole_footer = (kheap_block_footer_t*)(new_block_end - FOOTER_SIZE);
    // hole_footer->magic = HEAP_MAGIC;
    // hole_footer->header = new_hole_header;

    header_pos = header_pos + cutSize;
    block_size = block_size - cut_hole_header->size;
  } else {
    ordered_array_remove(&kheap->index, iterator);
  }

  // use this block
  kheap_block_header_t* selected_hole_header = make_block(header_pos, requested_size, NOT_HOLE);

  // kheap_block_header_t* block_header = (kheap_block_header_t*)header_pos;
  // block_header->magic = HEAP_MAGIC;
  // block_header->is_hole = 0;
  // block_header->size = requested_size;

  // kheap_block_footer_t* block_footer = (kheap_block_footer_t*)(header_pos + HEADER_SIZE + size);
  // block_footer->magic = HEAP_MAGIC;
  // block_footer->header = block_header;

  // We may need to write a new hole after the allocated block.
  // We do this only if the new hole would have positive size...
  uint32 remain_size = block_size - requested_size;
  ASSERT(remain_size >= 0);
  if (remaining_size > 0) {
    ASSERT(remain_size > BLOCK_META_SIZE);

    kheap_block_header_t* remain_hole_header =
        make_block(header_pos + BLOCK_META_SIZE + size, remaining_size, IS_HOLE);

    // kheap_block_header_t *new_hole_header =
    //     (kheap_block_header_t*)(header_pos + BLOCK_META_SIZE + size);
    // new_hole_header->magic = HEAP_MAGIC;
    // new_hole_header->is_hole = 1;
    // new_hole_header->size = remaining_size;

    // kheap_block_footer_t *new_hole_footer =
    //     (kheap_block_footer_t*)((uint32)new_hole_header + remaining_size - FOOTER_SIZE);
    // // TODO: ?
    // if ((uint32)new_hole_footer < heap->end_address) {
    //   new_hole_footer->magic = HEAP_MAGIC;
    //   new_hole_footer->header = new_hole_header;
    // }

    // Insert the new hole into heap.
    ordered_array_insert(heap->index, (void*)remain_hole_header);
  }
  
  // done
  return (void*)((uint32)selected_hole_header + HEADER_SIZE);
}

void free(void *p, heap_t *heap)
{
  // Exit gracefully for null pointers.
  if (p == 0)
    return;

  // Get the header and footer associated with this pointer.
  header_t *header = (header_t*) ( (uint32)p - sizeof(header_t) );
  footer_t *footer = (footer_t*) ( (uint32)header + header->size - sizeof(footer_t) );

  // Sanity checks.
  ASSERT(header->magic == HEAP_MAGIC);
  ASSERT(footer->magic == HEAP_MAGIC);

  // Make us a hole.
  header->is_hole = 1;

  // Do we want to add this header into the 'free holes' index?
  char do_add = 1;

  // Unify left
  // If the thing immediately to the left of us is a footer...
  footer_t *test_footer = (footer_t*) ( (uint32)header - sizeof(footer_t) );
  if (test_footer->magic == HEAP_MAGIC &&
    test_footer->header->is_hole == 1)
  {
    uint32 cache_size = header->size; // Cache our current size.
    header = test_footer->header;   // Rewrite our header with the new one.
    footer->header = header;      // Rewrite our footer to point to the new header.
    header->size += cache_size;     // Change the size.
    do_add = 0;             // Since this header is already in the index, we don't want to add it again.
  }

  // Unify right
  // If the thing immediately to the right of us is a header...
  header_t *test_header = (header_t*) ( (uint32)footer + sizeof(footer_t) );
  if (test_header->magic == HEAP_MAGIC &&
    test_header->is_hole)
  {
    header->size += test_header->size; // Increase our size.
    test_footer = (footer_t*) ( (uint32)test_header + // Rewrite it's footer to point to our header.
                  test_header->size - sizeof(footer_t) );
    footer = test_footer;
    // Find and remove this header from the index.
    uint32 iterator = 0;
    while ( (iterator < heap->index.size) &&
        (lookup_ordered_array(iterator, &heap->index) != (void*)test_header) )
      iterator++;

    // Make sure we actually found the item.
    ASSERT(iterator < heap->index.size);
    // Remove it.
    remove_ordered_array(iterator, &heap->index);
  }

  // If the footer location is the end address, we can contract.
  if ( (uint32)footer+sizeof(footer_t) == heap->end_address)
  {
    uint32 old_length = heap->end_address-heap->start_address;
    uint32 new_length = contract( (uint32)header - heap->start_address, heap);
    // Check how big we will be after resizing.
    if (header->size - (old_length-new_length) > 0)
    {
      // We will still exist, so resize us.
      header->size -= old_length-new_length;
      footer = (footer_t*) ( (uint32)header + header->size - sizeof(footer_t) );
      footer->magic = HEAP_MAGIC;
      footer->header = header;
    }
    else
    {
      // We will no longer exist :(. Remove us from the index.
      uint32 iterator = 0;
      while ( (iterator < heap->index.size) &&
          (lookup_ordered_array(iterator, &heap->index) != (void*)test_header) )
        iterator++;
      // If we didn't find ourselves, we have nothing to remove.
      if (iterator < heap->index.size)
        remove_ordered_array(iterator, &heap->index);
    }
  }

  // If required, add us to the index.
  if (do_add == 1)
    insert_ordered_array((void*)header, &heap->index);

}
