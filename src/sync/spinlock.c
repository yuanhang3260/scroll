#include "sync/spinlock.h"
#include "task/thread.h"
#include "task/scheduler.h"

extern uint32 atomic_exchange(volatile uint32* dst, uint32 src);

void spinlock_init(spinlock_t* splock) {
  splock->hold = LOCKED_NO;
}

void spinlock_lock(spinlock_t *splock) {
  while (atomic_exchange(&splock->hold , LOCKED_YES) != LOCKED_NO) {
    // on single-core processor, yield this thread.
    #ifdef SINGLE_PROCESSOR
      schedule_thread_yield();
    #else
      // TODO: on multi-core processor, just loop spinning.
    #endif
  }
}

bool spinlock_trylock(spinlock_t* splock) {
  return (atomic_exchange(&splock->hold , LOCKED_YES) == LOCKED_NO);
}

void spinlock_unlock(spinlock_t *splock) {
  splock->hold = LOCKED_NO;
}
