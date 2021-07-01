#ifndef SYNC_YIELDLOCK_H
#define SYNC_YIELDLOCK_H

#include "common/common.h"

#define LOCKED_YES 1
#define LOCKED_NO 0

#define SINGLE_PROCESSOR

typedef struct yieldlock {
  volatile uint32 hold;
} yieldlock_t;

// ****************************************************************************
void yieldlock_init(yieldlock_t* splock);
void yieldlock_lock(yieldlock_t* splock);
bool yieldlock_trylock(yieldlock_t* splock);
void yieldlock_unlock(yieldlock_t* splock);

#endif
