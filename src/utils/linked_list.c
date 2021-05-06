#include "utils/linked_list.h"

linked_list_t create_linked_list() {
  linked_list_t linked_list;
  linked_list.head = 0;
  linked_list.tail = 0;
  linked_list.size = 0;
  return linked_list;
}

void linked_list_append(linked_list_t* this, linked_list_node_t* new_node) {
  new_node->next = 0;
  if (this->size == 0) {
    // Add first node.
    new_node->prev = 0;
    this->head = new_node;
    this->tail = new_node;
  } else {
    linked_list_node_t* tail = this->tail;
    tail->next = new_node;
    new_node->prev = tail;
    this->tail = new_node;
  }

  this->size++;
}

// insert after node.
void linked_list_insert(linked_list_t* this, linked_list_node_t* node, linked_list_node_t* new_node) {
  new_node->prev = node;

  linked_list_node_t* next_node;
  if (node == 0) {
    // Insert to head.
    next_node = this->head;
  } else {
    next_node = node->next;
    node->next = new_node;
  }

  new_node->next = next_node;
  if (next_node != 0) {
    next_node->prev = new_node;
  } else {
    // If insert after tail.
    this->tail = new_node;
  }

  this->size++;
}

void linked_list_remove(linked_list_t* this, linked_list_node_t* node) {
  linked_list_node_t* prev_node = node->prev;
  linked_list_node_t* next_node = node->next;
  if (prev_node != 0) {
    prev_node->next = next_node;
  } else {
    // If head is removed.
    this->head = next_node;
  }
  if (next_node != 0) {
    next_node->prev = prev_node;
  } else {
    // If tail is removed.
    this->tail = prev_node;
  }

  this->size--;
}
