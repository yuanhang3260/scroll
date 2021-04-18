#ifndef UTILS_BITMAP_H
#define UTILS_BITMAP_H

#include "common/common.h"
#include "common/global.h"

// argument struct for common isr_handler
typedef struct bit_map {
  uint32* array;
  int array_size;  // size of the array
  int total_bits;
} bitmap_t;


void set_bit(bitmap_t* bitmap, uint32 bit);

void clear_bit(bitmap_t* bitmap, uint32 bit);

uint32 test_bit(bitmap_t* bitmap, uint32 bit);

// Returns sucess or not.
// The result bit is stored in argument *bit.
uint32 find_first_free(bitmap_t* bitmap, uint32* bit);
uint32 allocate_first_free(bitmap_t* bitmap, uint32* bit);

#endif
