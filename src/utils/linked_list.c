#include "monitor/monitor.h"
#include "mem/kheap.h"
#include "utils/debug.h"
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
void linked_list_insert(
    linked_list_t* this, linked_list_node_t* node, linked_list_node_t* new_node) {
  new_node->prev = node;

  linked_list_node_t* next_node;
  if (node == 0) {
    // Insert to head.
    next_node = this->head;
    this->head = new_node;
  } else {
    next_node = node->next;
    node->next = new_node;
  }

  new_node->next = next_node;
  if (next_node != 0) {
    next_node->prev = new_node;
  } else {
    // Insert after tail.
    this->tail = new_node;
  }

  this->size++;
}

void linked_list_insert_to_head(linked_list_t* this, linked_list_node_t* new_node) {
  linked_list_insert(this, this->head, new_node);  
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


// ****************************************************************************
void linked_list_test() {
  monitor_printf("linked_list test ... ");

  linked_list_t linked_list = create_linked_list();

  // Insert first node.
  linked_list_node_t* node = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  linked_list_append(&linked_list, node);
  ASSERT(linked_list.size == 1);
  ASSERT(linked_list.head == node);
  ASSERT(linked_list.tail == node);
  ASSERT(node->prev == 0);
  ASSERT(node->next == 0);

  // Remove it.
  linked_list_remove(&linked_list, node);
  ASSERT(linked_list.size == 0);
  ASSERT(linked_list.head == 0);
  ASSERT(linked_list.tail == 0);

  // Insert again.
  linked_list_append(&linked_list, node);

  // Insert second node.
  linked_list_node_t* node2 = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  linked_list_append(&linked_list, node2);
  ASSERT(linked_list.size == 2);
  ASSERT(linked_list.head == node);
  ASSERT(linked_list.tail == node2);
  ASSERT(node->prev == 0);
  ASSERT(node->next == node2);
  ASSERT(node2->prev == node);
  ASSERT(node2->next == 0);

  // Insert thrid node.
  linked_list_node_t* node3 = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  linked_list_append(&linked_list, node3);
  ASSERT(linked_list.size == 3);
  ASSERT(linked_list.head == node);
  ASSERT(linked_list.tail == node3);
  ASSERT(node->prev == 0);
  ASSERT(node->next == node2);
  ASSERT(node2->prev == node);
  ASSERT(node2->next == node3);
  ASSERT(node3->prev == node2);
  ASSERT(node3->next == 0);

  // Insert 4th node after node 2.
  linked_list_node_t* node4 = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  linked_list_insert(&linked_list, node2, node4);
  ASSERT(linked_list.size == 4);
  ASSERT(linked_list.head == node);
  ASSERT(linked_list.tail == node3);
  ASSERT(node->prev == 0);
  ASSERT(node->next == node2);
  ASSERT(node2->prev == node);
  ASSERT(node2->next == node4);
  ASSERT(node4->prev == node2);
  ASSERT(node4->next == node3);
  ASSERT(node3->prev == node4);
  ASSERT(node3->next == 0);

  // Remove 2nd node.
  linked_list_remove(&linked_list, node2);
  ASSERT(linked_list.size == 3);
  ASSERT(linked_list.head == node);
  ASSERT(linked_list.tail == node3);
  ASSERT(node->prev == 0);
  ASSERT(node->next == node4);
  ASSERT(node4->prev == node);
  ASSERT(node4->next == node3);
  ASSERT(node3->prev == node4);
  ASSERT(node3->next == 0);

  // Remove 1st node.
  linked_list_remove(&linked_list, node);
  ASSERT(linked_list.size == 2);
  ASSERT(linked_list.head == node4);
  ASSERT(linked_list.tail == node3);
  ASSERT(node4->prev == 0);
  ASSERT(node4->next == node3);
  ASSERT(node3->prev == node4);
  ASSERT(node3->next == 0);

  // Remove 3rd node.
  linked_list_remove(&linked_list, node3);
  ASSERT(linked_list.size == 1);
  ASSERT(linked_list.head == node4);
  ASSERT(linked_list.tail == node4);

  // Insert 5th node to head.
  linked_list_node_t* node5 = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  linked_list_insert(&linked_list, 0, node5);
  ASSERT(linked_list.size == 2);
  ASSERT(linked_list.head == node5);
  ASSERT(linked_list.tail == node4);
  ASSERT(node5->prev == 0);
  ASSERT(node5->next == node4);
  ASSERT(node4->prev == node5);
  ASSERT(node4->next == 0);

  // Remove 4th node.
  linked_list_remove(&linked_list, node4);
  ASSERT(linked_list.size == 1);
  ASSERT(linked_list.head == node5);
  ASSERT(linked_list.tail == node5);

  // Remove 5th node.
  linked_list_remove(&linked_list, node5);
  ASSERT(linked_list.size == 0);
  ASSERT(linked_list.head == 0);
  ASSERT(linked_list.tail == 0);

  // Remove 6th node to empty list.
  linked_list_node_t* node6 = (linked_list_node_t*)kmalloc(sizeof(linked_list_node_t));
  linked_list_insert(&linked_list, 0, node6);
  ASSERT(linked_list.size == 1);
  ASSERT(linked_list.head == node6);
  ASSERT(linked_list.tail == node6);

  // Remove 6th node.
  linked_list_remove(&linked_list, node6);
  ASSERT(linked_list.size == 0);
  ASSERT(linked_list.head == 0);
  ASSERT(linked_list.tail == 0);

  kfree(node);
  kfree(node2);
  kfree(node3);
  kfree(node4);
  kfree(node5);
  kfree(node6);

  monitor_print_with_color("OK\n", COLOR_GREEN);
  ASSERT(kheap_validate_print(1) == 0);
}
