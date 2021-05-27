#ifndef SYNC_SPINLOCK_H
#define SYNC_SPINLOCK_H

#include "common/common.h"

#define LOCKED_YES 1
#define LOCKED_NO 0

#define SINGLE_PROCESSOR

typedef struct spinlock {
  volatile uint32 hold;
} spinlock_t;

// ****************************************************************************
void spinlock_init(spinlock_t* splock);
void spinlock_lock(spinlock_t* splock);
bool spinlock_trylock(spinlock_t* splock);
void spinlock_unlock(spinlock_t* splock);

#endif
