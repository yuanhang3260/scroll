%include "src/boot/boot.inc"

SECTION loader vstart=LOADER_BASE_ADDR

loader_entry:
  jmp loader_start

; initialize gdt
GDT_BASE:
  dd 0x00000000
  dd 0x00000000

CODE_DESC:
  dd DESC_CODE_LOW_32
  dd DESC_CODE_HIGH_32

DATA_DESC:
  dd DESC_DATA_LOW_32
  dd DESC_DATA_HIGH_32

VIDEO_DESC:
  dd DESC_VIDEO_LOW_32
  dd DESC_VIDEO_HIGH_32

; reserve 64 gdt entries space
times 64 dq 0

GDT_SIZE    equ   $ - GDT_BASE
GDT_LIMIT   equ   GDT_SIZE - 1

SELECTOR_CODE    equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA    equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO   equ (0x0003 << 3) + TI_GDT + RPL0

gdt_ptr:
  dw GDT_LIMIT
  dd GDT_BASE


loader_start:
  call clear_screen
  call setup_protection_mode

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

print_message:
  ; position
  mov dx, 0x00

  ; print message
  mov bp, ax
  mov ah, 0x13  ; int num
  mov al, 0x01
  mov bh, 0x00  ; page number
  mov bl, 0x07
  int 0x10
  ret

setup_protection_mode:
  mov ax, message1
  mov cx, message1_length
  call print_message

  ; enable A20
  in al, 0x92
  or al, 0000_0010b
  out 0x92, al

  ; load GDT
  lgdt [gdt_ptr]

  ; open protection mode - set cr0 bit 0
  mov eax, cr0
  or eax, 0x00000001
  mov cr0, eax

  ; refresh pipeline
  jmp dword SELECTOR_CODE:protection_mode_start

[bits 32]
protection_mode_start:
  ; set data segments
  mov ax, SELECTOR_DATA
  mov ds, ax
  mov es, ax
  mov ss, ax

  ; set video segment
  mov ax, SELECTOR_VIDEO
  mov gs, ax

  ; let's test video memory - a green [OK] after message1
  mov byte [gs:message1_length*2+0], '['
  mov byte [gs:message1_length*2+2], 'O'
  mov byte [gs:message1_length*2+3], 0010b
  mov byte [gs:message1_length*2+4], 'K'
  mov byte [gs:message1_length*2+5], 0010b
  mov byte [gs:message1_length*2+6], ']'

  ret

setup_page:
  ; clear the memory space for page directory
  mov eax, PAGE_DIR_TABLE_POS
  mov ecx, 4096
  mov esi, 0
  call clear_memory

  ; create page directory
.create_pde: ; 创建 Page Directory Entry
  ; first pde
  mov eax, PAGE_DIR_TABLE_POS + 0x1000
  mov ebx, eax

  or eax, PG_US_U | PG_RW_W | PG_P
  mov [PAGE_DIR_TABLE_POS + 0], eax
  mov [PAGE_DIR_TABLE_POS + 768 * 4], eax

  ; the last pde
  mov eax, PAGE_DIR_TABLE_POS
  mov [PAGE_DIR_TABLE_POS + 4092], eax

  ; 创建内核其他页表的 PDE
  mov eax, PAGE_DIR_TABLE_POS
  add eax, 0x2000
  or eax, PG_US_U | PG_RW_W | PG_P
  mov ecx, 254
  mov esi, 769
.create_kernel_pde:
  mov [PAGE_DIR_TABLE_POS + esi * 4], eax
  inc esi
  add eax, 0x1000
  loop .create_kernel_pde

  ; create the first page table
  mov ecx, 256
  mov esi, 0
  mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
  mov [ebx + esi * 4], edx
  add edx, 4096
  inc esi
  loop .create_pte

  ret

clear_memory:
  mov byte [eax + esi], 0
  inc esi
  loop clear_memory
  ret


message1:
  db "setup protection mode ... "

message1_length equ $ - message1
