#include "driver/keyboard.h"
#include "driver/keyhelp.h"
#include "common/io.h"
#include "common/stdlib.h"
#include "monitor/monitor.h"
#include "sync/yieldlock.h"
#include "task/scheduler.h"
#include "interrupt/interrupt.h"
#include "utils/debug.h"
#include "utils/linked_list.h"

#define KEYBOARD_BUF_SIZE 1024

typedef struct {
  uint8 buffer[KEYBOARD_BUF_SIZE];
  int head;
  int tail;
  int size;
} buffer_queue_t;

static buffer_queue_t queue;

static linked_list_t waiting_tasks;

static yieldlock_t keyboard_lock;

static int next_index(int index) {
  return (index + 1) % KEYBOARD_BUF_SIZE;
}

static int enqueue(uint8 scanCode) { 
  if (queue.size == KEYBOARD_BUF_SIZE) {
    return -1;
  }

  // insert the new scancode
  queue.buffer[queue.tail] = scanCode;
  queue.tail = next_index(queue.tail);
  queue.size++;
  return 0;
}

static uint8 dequeue() {
  uint8 code = queue.buffer[queue.head];
  queue.head = next_index(queue.head);
  queue.size--;
  return code;
}

static int32 read_keyboard_char_impl() {
  if (queue.size == 0) {
    return -1;
  }

  int32 augchar = process_scancode((int)dequeue());
  while (!(KH_HASDATA(augchar) && KH_ISMAKE(augchar))) {
    if (queue.size == 0) {
      return -1;
    }
    augchar = process_scancode((int)dequeue());
  }
  return KH_GETCHAR(augchar);
}

int32 read_keyboard_char() {
  yieldlock_lock(&keyboard_lock);
  int32 c;
  while (1) {
    c = read_keyboard_char_impl();
    if (c != -1) {
      break;
    }
    //monitor_printf("keyboard waiting thread %u\n", get_crt_thread()->id);
    linked_list_append(&waiting_tasks, get_crt_thread_node());
    schedule_mark_thread_block();
    yieldlock_unlock(&keyboard_lock);
    schedule_thread_yield();

    yieldlock_lock(&keyboard_lock);
  }
  yieldlock_unlock(&keyboard_lock);
  return c;
}

static void keyboard_interrupt_handler() {
  uint8 scancode = inb(0x60);

  // Disable interrupts because this section of code is not reentrant.
  disable_interrupt();

  yieldlock_lock(&keyboard_lock);
  enqueue(scancode);
  linked_list_t waiting_tasks_get;
  linked_list_move(&waiting_tasks_get, &waiting_tasks);
  yieldlock_unlock(&keyboard_lock);

  enable_interrupt();

  thread_node_t* node = waiting_tasks_get.tail;
  while (node != nullptr) {
    thread_node_t* prev_node = node->prev;
    linked_list_remove(&waiting_tasks_get, node);
    //monitor_printf("wake up keyboard waiting thread %u\n", ((tcb_t*)node->ptr)->id);
    add_thread_node_to_schedule_head(node);
    //break;
    node = prev_node;
  }

  // To achieve faster keyboard response, do not waste time on kernel main thread.
  if (is_kernel_main_thread()) {
    schedule_thread_yield();
  }
}

void init_keyboard() {
  yieldlock_init(&keyboard_lock);
  linked_list_init(&waiting_tasks);

  queue.head = 0;
  queue.tail = 0;
  queue.size = 0;

  register_interrupt_handler(IRQ1_INT_NUM, &keyboard_interrupt_handler);
}
