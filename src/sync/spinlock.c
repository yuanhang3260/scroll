#include "sync/spinlock.h"
#include "task/thread.h"
#include "task/scheduler.h"

extern atomic_exchange(uint32* dst, uint32 src);

void spinlock_lock(spinlock_t *spinlock) {
  while (atomic_exchange(&spinlock->hold , LOCKED_YES) != LOCKED_NO) {
    // on single-core processor, yield this thread.
    #ifdef SINGLE_PROCESSOR
      schedule_thread_yield();
    #else
      // TODO: on multi-core processor, just loop spinning.
    #endif
  }
}

void spinlock_unlock(spinlock_t *spinlock) {
  spinlock->hold = LOCKED_NO;
}
