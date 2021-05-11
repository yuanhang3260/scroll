[GLOBAL atomic_exchange]

atomic_exchange:
  mov ecx, [esp + 4]
  mov eax, [esp + 8]
  xchg [ecx], eax
  ret
