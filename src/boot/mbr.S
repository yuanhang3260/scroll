; mbr
;------------------------------------------------------------
%include "src/boot/boot.inc"
SECTION MBR vstart=0x7c00

mbr_entry:
  call init_segments
  call clear_screen
  call print_hello
  jmp $

init_segments:
  mov ax, cs
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax

  ; let's move stack to 0x7b00 :)
  ; we need to copy the current return address to new stack base
  mov bx, sp
  mov ax, [bx]
  mov [0x7b00], ax
  mov sp, 0x7b00
  ret

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
  ; print position
  mov dx, 0x0

  ; print message
  mov ax, message
  mov bp, ax
  mov cx, 9
  mov ax, 0x1301
  mov bx, 0x7
  int 0x10

load_loader_img:
  mov eax, LOADER_START_SECTOR  ; 0x02
  mov bx, LOADER_BASE_ADDR  ; 0x0900
  mov cx, 0x01
  call rd_disk_m_16

rd_disk_m_16:
  mov esi, eax
  mov di, cx

  mov dx, 0x01f2
  mov al, cl
  out dx, al


message db "hello mbr"
times 510-($-$$) db 0
db 0x55, 0xaa
