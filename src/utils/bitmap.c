#include "utils/bitmap.h"
#include "mem/kheap.h"

#define INDEX_FROM_BIT(a) (a/32)
#define OFFSET_FROM_BIT(a) (a%32)

bitmap_t bitmap_create(uint32* array, int total_bits) {
  bitmap_t ret;
  ret.total_bits = total_bits;
  ret.array_size = total_bits / 32;
  if (array == 0) {
    array = (uint32*)kmalloc(ret.array_size);
    ret.alloc_array = 1;
  } else {
    ret.alloc_array = 0;
  }
  ret.array = array;
  return ret;
}

void bitmap_set_bit(bitmap_t* this, uint32 bit) {
  uint32 idx = INDEX_FROM_BIT(bit);
  uint32 off = OFFSET_FROM_BIT(bit);
  this->array[idx] |= (0x1 << off);
}

void bitmap_clear_bit(bitmap_t* this, uint32 bit) {
  uint32 idx = INDEX_FROM_BIT(bit);
  uint32 off = OFFSET_FROM_BIT(bit);
  this->array[idx] &= ~(0x1 << off);
}

uint32 bitmap_test_bit(bitmap_t* this, uint32 bit) {
  uint32 idx = INDEX_FROM_BIT(bit);
  uint32 off = OFFSET_FROM_BIT(bit);
  return (this->array[idx] & (0x1 << off));
}

uint32 bitmap_find_first_free(bitmap_t* this, uint32* bit) {
  uint32 i, j;
  for (i = 0; i < this->array_size; i++) {
    uint32 ele = this->array[i];
    if (ele != 0xFFFFFFFF) {
      for (j = 0; j < 32; j++) {
        if (!(ele & (0x1 << j))) {
          *bit = i * 32 + j;
          return 1;
        }
      }
    }
  }

  return 0;
}

uint32 bitmap_allocate_first_free(bitmap_t* this, uint32* bit) {
  uint32 success = bitmap_find_first_free(this, bit);
  if (!success) {
    return 0;
  }

  bitmap_set_bit(this, *bit);
  return 1;
}

void bitmap_destroy(bitmap_t* this) {
  if (this->alloc_array) {
    kfree(this->array);
  }
  kfree(this);
}
