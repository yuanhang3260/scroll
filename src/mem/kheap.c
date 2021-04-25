#include "mem/kheap.h"
#include "mem/paging.h"
#include "utils/debug.h"

static kheap_t *kheap = 0;

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

static void expand(kheap_t *this, uint32 new_size) {
  // Sanity check.
  ASSERT(new_size > this->end_address - this->start_address);

  // Make it page aligned.
  if (new_size & 0xFFFFF000 != 0) {
    new_size &= 0xFFFFF000;
    new_size += PAGE_SIZE;
  }

  uint32 new_end = this->start_address + new_size;
  ASSERT(new_end <= this->max_address);
  this->end_address = new_end;
}

static uint32 contract(kheap_t *this, uint32 new_size) {
  // Sanity check.
  ASSERT(new_size < this->end_address - this->start_address);

  // Get the nearest following page boundary.
  if (new_size & PAGE_SIZE) {
    new_size &= PAGE_SIZE;
    new_size += PAGE_SIZE;
  }

  if (new_size < HEAP_MIN_SIZE) {
    new_size = HEAP_MIN_SIZE;
  }

  this->end_address = this->start_address + new_size;
  return new_size;
}

// Find the smallest hole that will fit.
static int32 find_smallest_hole(kheap_t *this, uint32 size, uint8 page_align) {
  uint32 iterator = 0;
  for (int i = 0; i < this->index.size; i++) {
    kheap_block_header_t* header =
        (kheap_block_header_t*)ordered_array_get(&this->index, i);
    if (page_align) {
      // align the starting point.
      uint32 start = (uint32)header + sizeof(kheap_block_header_t);
      uint32 offset = 0;
      if (start & 0xFFFFF000 != 0) {
        offset = PAGE_SIZE - start % PAGE_SIZE;
      }

      int32 hole_size = (int32)header->size - offset;
      if (hole_size >= (int32)size) {
        return i;
      }
    } else if (header->size >= size) {
      return i;
    }
  }

  return -1;
}

static int8 kheap_block_comparator(void* x, void * y) {
  uint32 size1 = ((kheap_block_header_t*)x)->size;
  uint32 size2 = ((kheap_block_header_t*)y)->size;
  return  size1 < size2 ? -1 : (size1 == size2 ? 0 : 1);
}

kheap_t *create_kheap(
    uint32 start, uint32 end_addr, uint32 max, uint8 supervisor, uint8 readonly) {
  ASSERT(start % PAGE_SIZE == 0);
  ASSERT(end_addr % PAGE_SIZE == 0);

  kheap_t kheap;
  
  // Initialise the index.
  kheap.index = place_ordered_array( (void*)start, HEAP_INDEX_NUM, &header_t_less_than);
  
  // Shift the start address forward to resemble where we can start putting data.
  start += (sizeof(type_t) * HEAP_INDEX_NUM);

  // Write the start, end and max addresses into the heap structure.
  heap->start_address = start;
  heap->end_address = end_addr;
  heap->max_address = max;
  heap->supervisor = supervisor;
  heap->readonly = readonly;

  // Start off with one large hole in the index.
  kheap_block_header_t* hole = (kheap_block_header_t*)start;
  hole->size = end_addr - start;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = 1;
  ordered_array_insert(&kheap->index, (void*)hole);

  return kheap_t;
}

void *alloc(uint32 size, uint8 page_align, heap_t *heap) {
  // Make sure we take the size of header/footer into account.
  uint32 new_size = size + sizeof(header_t) + sizeof(footer_t);
  // Find the smallest hole that will fit.
  int32 iterator = find_smallest_hole(new_size, page_align, heap);

  if (iterator == -1) // If we didn't find a suitable hole
  {
    // Save some previous data.
    uint32 old_length = heap->end_address - heap->start_address;
    uint32 old_end_address = heap->end_address;

    // We need to allocate some more space.
    expand(old_length+new_size, heap);
    uint32 new_length = heap->end_address-heap->start_address;

    // Find the endmost header. (Not endmost in size, but in location).
    iterator = 0;
    // Vars to hold the index of, and value of, the endmost header found so far.
    uint32 idx = -1; uint32 value = 0x0;
    while (iterator < heap->index.size)
    {
      uint32 tmp = (uint32)lookup_ordered_array(iterator, &heap->index);
      if (tmp > value)
      {
        value = tmp;
        idx = iterator;
      }
      iterator++;
    }

    // If we didn't find ANY headers, we need to add one.
    if (idx == -1)
    {
      header_t *header = (header_t *)old_end_address;
      header->magic = HEAP_MAGIC;
      header->size = new_length - old_length;
      header->is_hole = 1;
      footer_t *footer = (footer_t *) (old_end_address + header->size - sizeof(footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
      insert_ordered_array((void*)header, &heap->index);
    }
    else
    {
      // The last header needs adjusting.
      header_t *header = lookup_ordered_array(idx, &heap->index);
      header->size += new_length - old_length;
      // Rewrite the footer.
      footer_t *footer = (footer_t *) ( (uint32)header + header->size - sizeof(footer_t) );
      footer->header = header;
      footer->magic = HEAP_MAGIC;
    }
    // We now have enough space. Recurse, and call the function again.
    return alloc(size, page_align, heap);
  }

  header_t *orig_hole_header = (header_t *)lookup_ordered_array(iterator, &heap->index);
  uint32 orig_hole_pos = (uint32)orig_hole_header;
  uint32 orig_hole_size = orig_hole_header->size;
  // Here we work out if we should split the hole we found into two parts.
  // Is the original hole size - requested hole size less than the overhead for adding a new hole?
  if (orig_hole_size-new_size < sizeof(header_t)+sizeof(footer_t))
  {
    // Then just increase the requested size to the size of the hole we found.
    size += orig_hole_size-new_size;
    new_size = orig_hole_size;
  }

  // If we need to page-align the data, do it now and make a new hole in front of our block.
  if (page_align && orig_hole_pos&0xFFFFF000)
  {
    uint32 new_location   = orig_hole_pos + PAGE_SIZE /* page size */ - (orig_hole_pos&0xFFF) - sizeof(header_t);
    header_t *hole_header = (header_t *)orig_hole_pos;
    hole_header->size   = PAGE_SIZE /* page size */ - (orig_hole_pos&0xFFF) - sizeof(header_t);
    hole_header->magic  = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    footer_t *hole_footer = (footer_t *) ( (uint32)new_location - sizeof(footer_t) );
    hole_footer->magic  = HEAP_MAGIC;
    hole_footer->header   = hole_header;
    orig_hole_pos     = new_location;
    orig_hole_size    = orig_hole_size - hole_header->size;
  }
  else
  {
    // Else we don't need this hole any more, delete it from the index.
    remove_ordered_array(iterator, &heap->index);
  }

  // Overwrite the original header...
  header_t *block_header  = (header_t *)orig_hole_pos;
  block_header->magic   = HEAP_MAGIC;
  block_header->is_hole   = 0;
  block_header->size    = new_size;
  // ...And the footer
  footer_t *block_footer  = (footer_t *) (orig_hole_pos + sizeof(header_t) + size);
  block_footer->magic   = HEAP_MAGIC;
  block_footer->header  = block_header;

  // We may need to write a new hole after the allocated block.
  // We do this only if the new hole would have positive size...
  if (orig_hole_size - new_size > 0)
  {
    header_t *hole_header = (header_t *) (orig_hole_pos + sizeof(header_t) + size + sizeof(footer_t));
    hole_header->magic  = HEAP_MAGIC;
    hole_header->is_hole  = 1;
    hole_header->size   = orig_hole_size - new_size;
    footer_t *hole_footer = (footer_t *) ( (uint32)hole_header + orig_hole_size - new_size - sizeof(footer_t) );
    if ((uint32)hole_footer < heap->end_address)
    {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->header = hole_header;
    }
    // Put the new hole in the index;
    insert_ordered_array((void*)hole_header, &heap->index);
  }
  
  // ...And we're done!
  return (void *) ( (uint32)block_header+sizeof(header_t) );
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
