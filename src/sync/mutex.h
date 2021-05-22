#ifndef SYNC_MUTEX_H
#define SYNC_MUTEX_H

#include "common/common.h"
#include "sync/spinlock.h"
#include "utils/linked_list.h"

// Mutex is a blocking lock. If thread can not acquire lock, it adds itself to mutex's waiting
// queue and wait for the lock releaser to wake up it.
struct mutex {
  volatile uint32 hold;
  volatile linked_list_t* thread_node;

  // Note we use a spinlock to protect wait task queue.
  linked_list_t waiting_task_queue;
  spinlock_t waiting_task_queue_lock;
};
typedef struct mutex mutex_t;


// ****************************************************************************
void mutex_init(mutex_t* mp);
void mutex_lock(mutex_t* mp);
void mutex_unlock(mutex_t* mp);


#endif
