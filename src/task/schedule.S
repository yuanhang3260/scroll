[GLOBAL cpu_idle]
[GLOBAL context_switch]

cpu_idle:
  hlt

context_switch:
  push esi
  push edi
  push ebx
  push ebp
  mov eax, [esp + 20]
  mov [eax], esp

  ; jump to next thread's kernel stack and recover its execution
  mov eax, [esp + 24]
  mov esp, [eax]
  pop ebp
  pop ebx
  pop edi
  pop esi

  sti
  ret
