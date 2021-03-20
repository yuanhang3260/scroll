SECTION loader vstart=0x900

loader_entry:
  call clear_screen
  call print_hello
  jmp $

clear_screen:
  mov byte ah, 0x06
  mov byte al, 0x00
  mov byte bh, 0x07
  ; start (0, 0)
  mov byte cl, 0x00
  mov byte ch, 0x00
  ; end: (dl, dh) = (x:79, y:24)
  mov byte dl, 0x4f
  mov byte dh, 0x18

  int 0x10
  ret

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
