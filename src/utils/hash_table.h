#ifndef UTILS_HASH_TABLE_H
#define UTILS_HASH_TABLE_H

#include "common/common.h"
#include "utils/linked_list.h"

// Hash table which only accepts uint32 as key.

struct hash_table_kv {
  uint32 key;
  void* v_ptr;
};
typedef struct hash_table_kv hash_table_kv_t;

struct hash_table {
  linked_list_t* buckets;
  uint32 buckets_num;
  uint32 size;
};
typedef struct hash_table hash_table_t;


// ****************************************************************************
hash_table_t create_hash_table();

void hash_table_init(hash_table_t* this);

// If the key already exists in map, old value will be replaced and returned.
void* hash_table_put(hash_table_t* this, uint32 key, void* v_ptr);

void* hash_table_get(hash_table_t* this, uint32 key);

bool hash_table_contains(hash_table_t* this, uint32 key);

void* hash_table_remove(hash_table_t* this, uint32 key);

void hash_table_destroy(hash_table_t* this);

void hash_table_print(hash_table_t* this);


// ****************************** unit test ***********************************
void hash_table_test();

#endif
