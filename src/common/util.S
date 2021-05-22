[GLOBAL get_eip]
get_eip:
  pop eax
  jmp eax

[GLOBAL get_ebp]
get_ebp:
  mov eax, ebp
  ret

[GLOBAL get_esp]
get_esp:
  mov eax, esp
  ret

[GLOBAL get_eax]
get_eax:
  ret
