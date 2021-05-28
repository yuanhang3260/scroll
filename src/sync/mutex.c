#include "sync/mutex.h"
#include "task/scheduler.h"

extern uint32 atomic_exchange(volatile uint32* dst, uint32 src);

void mutex_init(mutex_t* mp) {
  mp->hold = LOCKED_NO;
  mp->thread_node = nullptr;
  linked_list_init(&mp->waiting_task_queue);
  spinlock_init(&mp->splock);
}

void mutex_lock(mutex_t* mp) {
  spinlock_lock(&mp->splock);
  while (atomic_exchange(&mp->hold , LOCKED_YES) != LOCKED_NO) {
    // Add current thread to wait queue.
    thread_node_t* thread_node = get_crt_thread_node();
    linked_list_append(&mp->waiting_task_queue, thread_node);
    schedule_mark_thread_block();
    spinlock_unlock(&mp->splock);
    schedule_thread_yield();

    // Waken up, and try acquire lock again.
    spinlock_lock(&mp->splock);
  }
  spinlock_unlock(&mp->splock);
}

void mutex_unlock(mutex_t* mp) {
  spinlock_lock(&mp->splock);
  mp->hold = LOCKED_NO;
  mp->thread_node = nullptr;

  if (mp->waiting_task_queue.size != 0) {
    // Wake up waiting thread.
    thread_node_t* head = mp->waiting_task_queue.head;
    linked_list_remove(&mp->waiting_task_queue, head);
    add_thread_node_to_schedule(head);
  }
  spinlock_unlock(&mp->splock);
}
