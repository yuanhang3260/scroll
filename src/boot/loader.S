SECTION loader vstart=0x900

print_hello:
  ; position
  mov dx, 0x0

  ; print message
  mov ax, message
  mov bp, ax
  mov cx, 9
  mov ax, 0x1301
  mov bx, 0x7
  int 0x10

message db "hello mbr"
