[GLOBAL syscall_entry]

[EXTERN syscall_handler]

syscall_entry:
  ; disable interrupt
  cli

  ; push dummy to match struct isr_params_t
  push byte 0
  push byte 0

  ; save common registers
  push eax
  push ecx
  push edx
  push ebx
  push esp
  push ebp
  push esi
  push edi

  ; save original data segment
  mov cx, ds
  push ecx

  ; load the kernel data segment descriptor
  mov cx, 0x10
  mov ds, cx
  mov es, cx
  mov fs, cx
  mov gs, cx

  call syscall_handler

  ; recover the original data segment
  pop ecx
  mov ds, cx
  mov es, cx
  mov fs, cx
  mov gs, cx

  pop  edi
  pop  esi
  pop  ebp
  pop  esp
  pop  ebx
  pop  edx
  pop  ecx
  add esp, 4  ; skip eax because it is used as return value for syscall_handler

  ; pop dummy values
  add esp, 8

  ; make sure interrupt is enabled
  sti
  ; pop cs, eip, eflags, user_ss, and user_esp by processor
  iret
