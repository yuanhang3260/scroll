#include "common/common.h"
#include "common/stdio.h"
#include "syscall/syscall.h"
#include "fs/file.h"

int main(uint32 argc, char* argv[]) {
  listdir(".");
  return 0;
}
