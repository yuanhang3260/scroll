#include "utils/bitmap.h"

#define INDEX_FROM_BIT(a) (a/32)
#define OFFSET_FROM_BIT(a) (a%32)

static uint32 out_of_range(bitmap_t* bitmap, uint32 bit) {
  return bit >= bitmap->total_bits;
}

void set_bit(bitmap_t* bitmap, uint32 bit) {
  uint32 idx = INDEX_FROM_BIT(bit);
  uint32 off = OFFSET_FROM_BIT(bit);
  bitmap->array[idx] |= (0x1 << off);
}

void clear_bit(bitmap_t* bitmap, uint32 bit) {
  uint32 idx = INDEX_FROM_BIT(bit);
  uint32 off = OFFSET_FROM_BIT(bit);
  bitmap->array[idx] &= ~(0x1 << off);
}

uint32 test_bit(bitmap_t* bitmap, uint32 bit) {
  uint32 idx = INDEX_FROM_BIT(bit);
  uint32 off = OFFSET_FROM_BIT(bit);
  return (bitmap->array[idx] & (0x1 << off));
}

uint32 find_first_free(bitmap_t* bitmap, uint32* bit) {
  uint32 i, j;
  for (i = 0; i < bitmap->total_bits; i++) {
    uint32 ele = bitmap->array[i];
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

uint32 allocate_first_free(bitmap_t* bitmap, uint32* bit) {
  uint32 success = find_first_free(bitmap, bit);
  if (!success) {
    return 0;
  }

  set_bit(bitmap, *bit);
  return 1;
}
