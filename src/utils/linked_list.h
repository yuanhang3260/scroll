#ifndef UTILS_LINKED_LIST_H
#define UTILS_LINKED_LIST_H

#include "common/common.h"

struct linked_list_node {
  type_t ptr;
  struct linked_list_node* prev;
  struct linked_list_node* next;
} __attribute__((packed));
typedef struct linked_list_node linked_list_node_t;

struct linked_list {
  linked_list_node_t* head;
  linked_list_node_t* tail;
  uint32 size;
} __attribute__((packed));
typedef struct linked_list linked_list_t;

// ****************************************************************************
linked_list_t create_linked_list();

void linked_list_append(linked_list_t* this, linked_list_node_t* new_node);

// insert after node.
void linked_list_insert(linked_list_t* this, linked_list_node_t* n, linked_list_node_t* new_node);

void linked_list_remove(linked_list_t* this, linked_list_node_t* n);


#endif