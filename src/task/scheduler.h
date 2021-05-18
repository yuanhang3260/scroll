#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include "task/thread.h"

// Init scheduler.
void init_scheduler();

// Start running scheduler.
void start_scheduler();

// Get current running thread.
tcb_t* get_crt_thread();

void add_thread_to_schedule(tcb_t* thread);

// Called by timer interrupt handler.
void maybe_context_switch();

// Yield thread - give up cpu and move current thread to ready queue tail.
void schedule_thread_yield();

void schedule_thread_exit(int32 exit_code);
void schedule_thread_exit_normal();

#endif
