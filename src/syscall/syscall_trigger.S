SYSCALL_EXIT_NUM     equ  0
SYSCALL_FORK_NUM     equ  1
SYSCALL_EXEC_NUM     equ  2
SYSCALL_YIELD_NUM    equ  3

%macro DEFINE_SYSCALL_TRIGGER_0_PARAM 2
  [GLOBAL trigger_syscall_%1]
  trigger_syscall_%1:
    mov eax, %2
    int 0x80
    ret
%endmacro

%macro DEFINE_SYSCALL_TRIGGER_1_PARAM 2
  [GLOBAL trigger_syscall_%1]
  trigger_syscall_%1:
    mov eax, %2
    mov ecx, [esp + 4]
    int 0x80
    ret
%endmacro

%macro DEFINE_SYSCALL_TRIGGER_2_PARAM 2
  [GLOBAL trigger_syscall_%1]
  trigger_syscall_%1:
    mov eax, %2
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    int 0x80
    ret
%endmacro

%macro DEFINE_SYSCALL_TRIGGER_3_PARAM 2
  [GLOBAL trigger_syscall_%1]
  trigger_syscall_%1:
    mov eax, %2
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    mov ebx, [esp + 12]
    int 0x80
    ret
%endmacro

%macro DEFINE_SYSCALL_TRIGGER_4_PARAM 2
  [GLOBAL trigger_syscall_%1]
  trigger_syscall_%1:
    mov eax, %2
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    mov ebx, [esp + 12]
    mov esi, [esp + 16]
    int 0x80
    ret
%endmacro

%macro DEFINE_SYSCALL_TRIGGER_5_PARAM 2
  [GLOBAL trigger_syscall_%1]
  trigger_syscall_%1:
    mov eax, %2
    mov ecx, [esp + 4]
    mov edx, [esp + 8]
    mov ebx, [esp + 12]
    mov esi, [esp + 16]
    mov edi, [esp + 20]
    int 0x80
    ret
%endmacro

; *****************************************************************************
DEFINE_SYSCALL_TRIGGER_1_PARAM   exit,   SYSCALL_EXIT_NUM
DEFINE_SYSCALL_TRIGGER_0_PARAM   fork,   SYSCALL_FORK_NUM
DEFINE_SYSCALL_TRIGGER_3_PARAM   exec,   SYSCALL_EXEC_NUM
DEFINE_SYSCALL_TRIGGER_0_PARAM   yield,  SYSCALL_YIELD_NUM
