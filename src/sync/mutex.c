#include "sync/mutex.h"
#include "task/scheduler.h"
#include "mem/kheap.h"

extern uint32 atomic_exchange(volatile uint32* dst, uint32 src);

void mutex_init(mutex_t* mp) {
  mp->hold = LOCKED_NO;
  mp->thread_node = nullptr;
  mp->waiting_task_queue = create_linked_list();
  spinlock_init(&mp->waiting_task_queue_lock);
}

void mutex_lock(mutex_t* mp) {
  if (atomic_exchange(&mp->hold , LOCKED_YES) != LOCKED_NO) {
    // Add current thread to wait queue.
    thread_node_t* thread_node = get_crt_thread_node();
    spinlock_lock(&mp->waiting_task_queue_lock);
    linked_list_append(&mp->waiting_task_queue, thread_node);
    spinlock_unlock(&mp->waiting_task_queue_lock);
    schedule_thread_yield();
  } else {
    mp->hold = LOCKED_YES;
  }
}

void mutex_unlock(mutex_t* mp) {
  mp->hold = LOCKED_NO;
}
