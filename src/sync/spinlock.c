#include "interrupt/interrupt.h"
#include "sync/spinlock.h"
#include "task/thread.h"
#include "task/scheduler.h"
#include "utils/debug.h"

extern uint32 atomic_exchange(volatile uint32* dst, uint32 src);
extern uint32 get_eflags();

void spinlock_init(spinlock_t* splock) {
  splock->hold = LOCKED_NO;
}

void spinlock_lock(spinlock_t *splock) {
  while (atomic_exchange(&splock->hold , LOCKED_YES) != LOCKED_NO) {
    #ifdef SINGLE_PROCESSOR
      // On uni-processor, pure spinlock should not be used.
      PANIC();
    #else
      // On multi-processor, just loop spinning.
    #endif
  }
}

// This spin lock disables interrupt.
void spinlock_lock_irqsave(spinlock_t *splock) {
  // Save interrupt flag and disable local interrupt.
  uint32 eflags = get_eflags();
  splock->interrupt_mask = (eflags & (1 << 9));
  disable_interrupt();
  #ifdef SINGLE_PROCESSOR
    // On uni-processor, disabling interrupt is sufficient to ensure mutual exclusive.
  #else
    // On multi-processor, just loop spinning.
    while (atomic_exchange(&splock->hold , LOCKED_YES) != LOCKED_NO) {}
  #endif
}

void spinlock_unlock(spinlock_t *splock) {
  splock->hold = LOCKED_NO;
}

void spinlock_unlock_irqload(spinlock_t *splock) {
  splock->hold = LOCKED_NO;
  // If interrupt is previously enabled before irqsave lock, re-enable it again.
  if (splock->interrupt_mask) {
    enable_interrupt();
  }
}
