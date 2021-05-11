#ifndef SYNC_SPINLOCK_H
#define SYNC_SPINLOCK_H

#include "common/common.h"

#define LOCKED_YES 1
#define LOCKED_NO 0

#define SINGLE_PROCESSOR

typedef struct spinlock {
  uint32 hold;
  uint32 thread;
} spinlock_t;

// ****************************************************************************
void spinlock_lock(spinlock_t *mp);
void spinlock_unlock(spinlock_t *mp);

#endif
