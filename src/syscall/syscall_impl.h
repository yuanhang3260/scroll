#ifndef SYSCALL_SYSCALL_IMPL_H
#define SYSCALL_SYSCALL_IMPL_H

#include "common/common.h"
#include "interrupt/interrupt.h"

#define SYSCALL_EXIT_NUM     0
#define SYSCALL_FORK_NUM     1
#define SYSCALL_EXEC_NUM     2
#define SYSCALL_YIELD_NUM    3
#define SYSCALL_READ_NUM     4
#define SYSCALL_WRITE_NUM    5
#define SYSCALL_STAT_NUM     6
#define SYSCALL_LISTDIR_NUM  7
#define SYSCALL_PRINT_NUM    8
#define SYSCALL_WAIT_NUM     9

int32 syscall_handler(isr_params_t isr_params);


#endif
