#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include "task/thread.h"

// Init scheduler.
void init_scheduler();

// Start running scheduler.
void start_scheduler();

// Create a new thread and add it to ready queue.
tcb* create_thread(char* name, thread_func function, void* func_arg, uint32 priority);

// Called by timer interrupt handler.
void maybe_context_switch();

// Yield thread - give up cpu and move current thread to ready queue tail.
void thread_yield();

#endif
