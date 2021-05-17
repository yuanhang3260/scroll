#ifndef SYSCALL_SYSCALL_IMPL_H
#define SYSCALL_SYSCALL_IMPL_H

#include "common/common.h"
#include "interrupt/interrupt.h"

#define SYSCALL_EXIT_NUM     0
#define SYSCALL_FORK_NUM     1
#define SYSCALL_EXEC_NUM     2
#define SYSCALL_YIELD_NUM    3

int32 syscall_handler(isr_params_t isr_params);



#endif
