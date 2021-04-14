[GLOBAL get_eip]
get_eip:
  pop eax
  jmp eax

[GLOBAL get_ebp]
get_ebp:
  mov eax, ebp
  ret
