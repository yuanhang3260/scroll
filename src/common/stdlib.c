#include "common/stdlib.h"

void memset(void* addr, uint8 value, int num) {
  for (int i = 0; i < num; i++) {
    *((uint8*)addr) = value;
  }
}

void memcpy(void* dst, void* src, int num) {
  for (int i = 0; i < num; i++) {
    *((uint8*)(dst + i)) = *((uint8*)(src + i));
  }
}

void strcpy(char* dst, char* src) {
  while (*src != '\0') {
    *dst = *src;
    dst++;
    src++;
  }
}