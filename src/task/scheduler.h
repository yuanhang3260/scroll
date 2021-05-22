#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include "task/thread.h"

// Init scheduler.
void init_scheduler();

// Get current running thread.
tcb_t* get_crt_thread();
thread_node_t* get_crt_thread_node();

// Get process by id.
pcb_t* get_process(uint32 pid);

// Add thread to ready task queue and wait for schedule.
void add_thread_to_schedule(struct task_struct* thread);
void add_thread_node_to_schedule(thread_node_t* thread_node);

// Add process to scheduler
void add_process_to_schedule(pcb_t* process);

// Called by timer interrupt handler.
void maybe_context_switch();

// Yield thread - give up cpu and move current thread to ready queue tail.
void schedule_thread_yield();

void schedule_thread_exit(int32 exit_code);
void schedule_thread_exit_normal();

#endif
