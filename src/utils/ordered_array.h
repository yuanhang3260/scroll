#ifndef UTILS_ORDERED_ARRAY_H
#define UTILS_ORDERED_ARRAY_H

#include "common/common.h"

typedef void* type_t;

// An comparator function is used to sort elements. It returns -1, 0 or 1 if
// the first element is less than, equal to or greater than the second.
typedef int8 (*comparator_t)(type_t, type_t);

typedef struct ordered_array {
  type_t* array;
  uint32 size;
  uint32 max_size;
  comparator_t comparator;
} ordered_array_t;

int8 standard_comparator(type_t a, type_t b);

ordered_array_t ordered_array_create(type_t* array, uint32 max_size, comparator_t comparator);

// Return 1 for success, or 0 for failure
int32 ordered_array_insert(ordered_array_t *this, type_t item);

// Return result pointer, or 0 if i is out of bound.
type_t ordered_array_get(ordered_array_t *this, uint32 i);

void remove_ordered_array(ordered_array_t *this, uint32 i);

#endif
