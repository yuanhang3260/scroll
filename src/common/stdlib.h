#ifndef COMMON_STDLIB_H
#define COMMON_STDLIB_H

#include "common/common.h"

void memset(void* addr, uint8 value, int num);
void memcpy(void* dst, void* src, int num);
void strcpy(char* dst, char* src);

#endif
